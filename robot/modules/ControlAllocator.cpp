/**
 * @file ControlAllocator.cpp
 * @brief 实现低频轮式机器人使用的独立控制分配线程。
 */

#include "robot/modules/ControlAllocator.hpp"

#include "robot/os/Clock.hpp"
#include "robot/params/ParamManager.hpp"

#include <stdio.h>

/** 绑定手动控制、开关、参数和两类执行器 topic。 */
ControlAllocator::ControlAllocator()
    : _manualSubscription(manualControlTopic()),
      _switchSubscription(manualControlSwitchesTopic()),
      _parameterSubscription(parameterUpdateTopic()),
      _motorsPublication(actuatorMotorsTopic()),
      _servosPublication(actuatorServosTopic()),
      _armSwitch(1),
      _allocationCount(0),
      _lastArmWarning(0),
      _haveManual(false),
      _haveSwitches(false),
      _armed(false)
{
  (void)updateParameters();
}

/** 创建 control_allocator 独立任务。 */
int ControlAllocator::task_spawn(int argc, char *argv[])
{
  const int taskId = os::Task::spawn("control_allocator", 105, 4096,
                                     &run_trampoline, argc, argv);
  if (taskId < 0)
    {
      __atomic_store_n(&_taskId, -1, __ATOMIC_RELEASE);
      return -1;
    }

  __atomic_store_n(&_taskId, taskId, __ATOMIC_RELEASE);
  return wait_until_running();
}

/** 第一版仅接受 climbot 构型，后续增加机型时在此创建相应子类。 */
ControlAllocator *ControlAllocator::instantiate(int argc, char *argv[])
{
  for (int index = 0; index < argc; ++index)
    {
      if (argv[index] != nullptr && argv[index][0] == '-' &&
          argv[index][1] == 'c' && index + 1 < argc &&
          strcmp(argv[index + 1], "climbot") != 0)
        {
          fprintf(stderr, "unsupported robot configuration: %s\n",
                  argv[index + 1]);
          return nullptr;
        }
    }
  return new ControlAllocator();
}

/** 当前没有额外命令。 */
int ControlAllocator::custom_command(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  return print_usage("unknown command");
}

/** 打印任务启动和状态命令。 */
int ControlAllocator::print_usage(const char *reason)
{
  if (reason != nullptr)
    {
      fprintf(stderr, "%s\n", reason);
    }
  printf("usage: control_allocator {start [-c climbot]|stop|status}\n");
  return reason == nullptr ? 0 : -1;
}

/** 显示构型、分配次数和当前解锁状态。 */
int ControlAllocator::print_status()
{
  printf("running\n");
  printf("configuration: %s\n", _configuration.name());
  printf("motors/servos: %u/%u\n", _configuration.motorCount(),
         _configuration.servoCount());
  printf("arm switch: %ld\n", static_cast<long>(_armSwitch));
  printf("arm switch position: %u\n",
         static_cast<unsigned>(armSwitchPosition()));
  printf("arm safety: %s\n", _armSafety.stateName());
  printf("armed: %s\n", _armed ? "yes" : "no");
  printf("allocations: %lu\n", static_cast<unsigned long>(_allocationCount));
  return 0;
}

/** 以 50 Hz 检查新输入；任何输入或参数变化都会重新发布执行器输出。 */
void ControlAllocator::run()
{
  while (!should_exit())
    {
      bool changed = false;
      ParameterUpdate parameterUpdate;
      if (_parameterSubscription.update(parameterUpdate))
        {
          (void)updateParameters();
          changed = true;
        }

      if (_manualSubscription.update(_manual))
        {
          _haveManual = true;
          changed = true;
        }

      if (_switchSubscription.update(_switches))
        {
          _haveSwitches = true;
          changed = true;
        }

      if (changed)
        {
          _armed = updateArmState();
          ActuatorMotors motors{};
          ActuatorServos servos{};
          _allocation.allocate(_manual, _armed, motors, servos);
          const uint64_t now = os::Clock::nowMicroseconds();
          motors.timestamp = now;
          servos.timestamp = now;
          (void)_motorsPublication.publish(motors);
          (void)_servosPublication.publish(servos);
          ++_allocationCount;
        }

      warnIfArmSwitchUnsafe(os::Clock::nowMicroseconds());

      os::Clock::sleepMilliseconds(20);
    }
}

/** 返回 CA_ARM_SW 对应的位置；开关编号从 1 开始。 */
uint8_t ControlAllocator::armSwitchPosition() const
{
  if (!_haveSwitches || !_switches.valid || _armSwitch <= 0 ||
      _armSwitch > _switches.switchCount)
    {
      return ManualControlSwitches::POSITION_NONE;
    }

  return _switches.positions[_armSwitch - 1];
}

/** 遥控有效时更新SA电平；信号丢失只失能，不重复上电互锁。 */
bool ControlAllocator::updateArmState()
{
  const bool valid = _haveManual && _haveSwitches && _manual.valid &&
                     _switches.valid;
  return _armSafety.update(valid, armSwitchPosition());
}

/** 每五秒在控制台提示一次未通过上电安全检查的 SA 开关。 */
void ControlAllocator::warnIfArmSwitchUnsafe(uint64_t now)
{
  const uint8_t position = armSwitchPosition();
  if (!_armSafety.waitingForOff() ||
      position == ManualControlSwitches::POSITION_NONE ||
      position == ManualControlSwitches::POSITION_OFF)
    {
      return;
    }

  if (_lastArmWarning == 0 ||
      now - _lastArmWarning >= ArmWarningIntervalMicroseconds)
    {
      printf("WARNING [control_allocator] SA switch %ld is not OFF; "
             "set SA OFF before arming\n",
             static_cast<long>(_armSwitch));
      _lastArmWarning = now;
    }
}

/** 同时刷新算法系数和第几个遥控开关用于解锁。 */
bool ControlAllocator::updateParameters()
{
  int32_t armSwitch = 1;
  const bool allocationValid = _allocation.updateParameters();
  const bool switchValid = ParamManager::instance().get("CA_ARM_SW", armSwitch);
  if (switchValid)
    {
      if (_armSwitch != armSwitch)
        {
          _armSafety.reset();
          _armed = false;
          _lastArmWarning = 0;
        }
      _armSwitch = armSwitch;
    }
  return allocationValid && switchValid;
}
