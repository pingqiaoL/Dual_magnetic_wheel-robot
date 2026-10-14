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
#include <string.h>

/** 所有输出共享的NSH禁止标志；修改及读取均使用原子操作。 */
bool Command::_outputInhibited = false;
int32_t Command::_warningSwitch = 0;

/** 建立订阅并读取兼容的CA_ARM_SW参数。 */
Command::Command()
  : WorkQueueModuleBase<Command>(robot_wq::SafetyQueue(), 20000),
    ModuleParams(nullptr), _manualSubscription(manualControlTopic(), *this),
    _switchSubscription(manualControlSwitchesTopic(), *this),
    _parameterSubscription(parameterUpdateTopic(), *this),
    _armedPublication(actuatorArmedTopic()),
    _paramArmSwitch(this, "CA_ARM_SW", 1)
{
  (void)updateParams();
  updateParameters();
}



/** 创建独立安全管理对象。 */
Command *Command::instantiate(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  return new Command();
}

/** 输出安全管理命令的帮助。 */
int Command::print_usage(const char *reason)
{
  if (reason != nullptr) { fprintf(stderr, "%s\n", reason); }
  printf("usage: command {start|stop|status|enable|disable}\n");
  return reason == nullptr ? 0 : -1;
}

/** 处理手动输出禁止；解除禁止仍不能绕过遥控SA互锁。 */
int Command::custom_command(int argc, char *argv[])
{
  if (argc != 1 || (strcmp(argv[0], "enable") != 0 &&
                    strcmp(argv[0], "disable") != 0))
    { return print_usage("unknown command"); }
  const bool inhibit = strcmp(argv[0], "disable") == 0;
  setOutputInhibited(inhibit);
  printf(inhibit ? "output inhibit active\n"
                 : "output inhibit cleared; RC safety still required\n");
  return 0;
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
  int32_t armSwitch;
  uint8_t position;
  ActuatorArmed status{};
  const char *stateName;
  {
    os::LockGuard guard(_stateMutex);
    if (!guard.locked()) { return -1; }
    armSwitch = _armSwitch;
    position = _position;
    status = _status;
    stateName = _armSafety.stateName();
  }
  printf("running\narm switch: %ld\narm switch position: %u\n",
         static_cast<long>(armSwitch), position);
  printf("arm safety: %s\narmed: %s\nRC valid: %s\nlockdown: %s\n",
         stateName, status.armed ? "yes" : "no",
         status.valid ? "yes" : "no", outputInhibited() ? "yes" : "no");
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
  const int32_t mapping = _paramArmSwitch.get();
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
      __atomic_store_n(&_warningSwitch, _armSwitch, __ATOMIC_RELEASE);
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
      if (_parameterSubscription.update(update))
        {
          (void)updateParams();
          updateParameters();
        }
      (void)_manualSubscription.update(_manual);
      (void)_switchSubscription.update(_switches);
      if (now == 0) { now = os::Clock::nowMicroseconds(); }
      updateSafety(now);
    }
}

/** 50Hz刷新安全心跳；停任务后输出层会立即收到失锁消息。 */
bool Command::initWorkQueue()
{
  return _manualSubscription.registerCallback() &&
         _switchSubscription.registerCallback() &&
         _parameterSubscription.registerCallback();
}

void Command::unregisterCallbacks()
{
  _manualSubscription.unregisterCallback();
  _switchSubscription.unregisterCallback();
  _parameterSubscription.unregisterCallback();
}

void Command::Run()
{
  update();
}

void Command::onWorkQueueStopped()
{
  ActuatorArmed stopped{};
  stopped.timestamp = os::Clock::nowMicroseconds();
  (void)_armedPublication.publish(stopped);
}

// Called by RobotRuntime, never by the shared worker.
void Command::reportWarnings()
{
  const int32_t armSwitch = __atomic_exchange_n(&_warningSwitch, 0, __ATOMIC_ACQ_REL);
  if (armSwitch != 0)
    { printf("WARNING [command] switch %ld is not OFF; set OFF before arming\n",
             static_cast<long>(armSwitch)); }
}

/** NSH入口交给ModuleBase管理生命周期并回调本类扩展命令。 */
extern "C" int command_main(int argc, char *argv[])
{
  return Command::main(argc, argv);
}
