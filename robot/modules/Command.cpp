/** @file Command.cpp
 * @brief 将遥控安全开关和手动禁止状态转换为actuator_armed消息。
 */
#ifdef main
#undef main
#endif
#include "robot/modules/Command.hpp"
#include "robot/params/ParamManager.hpp"
#include "robot/os/Clock.hpp"
#include <stdio.h>

/** 所有输出共享的NSH禁止标志；修改及读取均使用原子操作。 */
bool Command::_outputInhibited = false;

/** 建立订阅并读取兼容的CA_ARM_SW参数。 */
Command::Command()
  : _manualSubscription(manualControlTopic()),
    _switchSubscription(manualControlSwitchesTopic()),
    _parameterSubscription(parameterUpdateTopic()),
    _armedPublication(actuatorArmedTopic())
{
  updateParameters();
}

/** 创建任务并等待对象就绪。 */
int Command::task_spawn(int argc, char *argv[])
{
  const int id = os::Task::spawn("command", 110, 4096,
                               &run_trampoline, argc, argv);
  __atomic_store_n(&_taskId, id < 0 ? -1 : id, __ATOMIC_RELEASE);
  return id < 0 ? -1 : wait_until_running();
}

/** 创建独立安全管理对象。 */
Command *Command::instantiate(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  return new Command();
}

/** 输出统一管理命令的帮助。 */
int Command::print_usage(const char *reason)
{
  if (reason != nullptr) { fprintf(stderr, "%s\n", reason); }
  printf("usage: command {start|stop|status|enable|disable|"
         "<module> <subcommand> [args]}\n");
  return reason == nullptr ? 0 : -1;
}

/** 原子修改禁止标志，使各任务不会读取到竞争写入。 */
void Command::setOutputInhibited(bool inhibited)
{
  __atomic_store_n(&_outputInhibited, inhibited, __ATOMIC_RELEASE);
}

/** 原子获取当前手动禁止标志。 */
bool Command::outputInhibited()
{
  return __atomic_load_n(&_outputInhibited, __ATOMIC_ACQUIRE);
}

/** 持状态锁打印安全检查、开关位置和解锁状态。 */
int Command::print_status()
{
  os::LockGuard guard(_stateMutex);
  if (!guard.locked()) { return -1; }
  printf("running\narm switch: %ld\narm switch position: %u\n",
         static_cast<long>(_armSwitch), _position);
  printf("arm safety: %s\narmed: %s\nRC valid: %s\nlockdown: %s\n",
         _armSafety.stateName(), _status.armed ? "yes" : "no",
         _status.valid ? "yes" : "no", outputInhibited() ? "yes" : "no");
  return 0;
}

/** 消息必须具有非零且不晚于当前时刻的时间戳。 */
bool Command::fresh(uint64_t timestamp, uint64_t now)
{
  return timestamp != 0 && timestamp <= now && now - timestamp <= 500000ULL;
}

/** 使用CA_ARM_SW选择rc_update已映射的第几路开关。 */
void Command::updateParameters()
{
  int32_t mapping = 0;
  (void)ParamManager::instance().get("CA_ARM_SW", mapping);
  if (_armSwitch != mapping)
    {
      _armSwitch = mapping;
      _armSafety.reset();
      _lastWarning = 0;
    }
}

/** 摇杆和开关都有效且未过期时才返回配置的开关位置。 */
uint8_t Command::switchPosition(uint64_t now) const
{
  if (!_manual.valid || !_switches.valid || _armSwitch < 1 ||
      _armSwitch > _switches.switchCount ||
      _armSwitch > ManualControlSwitches::SWITCH_COUNT ||
      !fresh(_manual.timestampSample, now) ||
      !fresh(_switches.timestampSample, now))
    { return ManualControlSwitches::POSITION_NONE; }
  const uint8_t position = _switches.positions[_armSwitch - 1];
  return position >= ManualControlSwitches::POSITION_ON &&
         position <= ManualControlSwitches::POSITION_OFF
           ? position : ManualControlSwitches::POSITION_NONE;
}

/** 每五秒提示尚未观察到OFF的开关；完成检查后不重复提示。 */
void Command::updateSafety(uint64_t now)
{
  _position = switchPosition(now);
  _status.timestamp = now;
  _status.valid = _position != ManualControlSwitches::POSITION_NONE;
  _status.lockdown = outputInhibited();
  _status.armed = _armSafety.update(_status.valid, _position) &&
                  !_status.lockdown;
  if (_armSafety.waitingForOff() && _status.valid &&
      _position != ManualControlSwitches::POSITION_OFF &&
      (_lastWarning == 0 || now - _lastWarning >= 5000000ULL))
    {
      printf("WARNING [command] switch %ld is not OFF; set OFF before arming\n",
             static_cast<long>(_armSwitch));
      _lastWarning = now;
    }
  (void)_armedPublication.publish(_status);
}

/** 状态锁保护单次更新，输入读取和消息发布始终使用同一份缓存。 */
void Command::update(uint64_t now)
{
  os::LockGuard guard(_stateMutex);
  if (guard.locked())
    {
      ParameterUpdate update{};
      if (_parameterSubscription.update(update)) { updateParameters(); }
      (void)_manualSubscription.update(_manual);
      (void)_switchSubscription.update(_switches);
      if (now == 0) { now = os::Clock::nowMicroseconds(); }
      updateSafety(now);
    }
}

/** 50Hz刷新安全心跳；停任务后输出层会立即收到失锁消息。 */
void Command::run()
{
  while (!should_exit())
    {
      update();
      os::Clock::sleepMilliseconds(20);
    }
  ActuatorArmed stopped{};
  stopped.timestamp = os::Clock::nowMicroseconds();
  (void)_armedPublication.publish(stopped);
}

/** NSH入口交给ModuleBase管理任务生命周期与统一命令路由。 */
extern "C" int command_main(int argc, char *argv[])
{
  return Command::main(argc, argv);
}
