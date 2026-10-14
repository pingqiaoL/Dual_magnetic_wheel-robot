/** @file RobotControl.cpp
 * @brief 将手动输入和独立Failsafe标志转换为机器人控制目标。
 */
#ifdef main
#undef main
#endif

#include "robot/modules/RobotControl.hpp"
#include "robot/os/Clock.hpp"

#include <math.h>
#include <stdio.h>

static constexpr uint64_t FailsafeStatusTimeoutUs = 500000ULL;

/** 建立输入订阅、目标发布和安全策略参数。 */
RobotControl::RobotControl()
  : WorkQueueModuleBase<RobotControl>(robot_wq::ControlQueue(), 20000),
    ModuleParams(nullptr), _manualSubscription(manualControlTopic(), *this),
    _failsafeSubscription(failsafeStatusTopic(), *this),
    _parameterSubscription(parameterUpdateTopic(), *this),
    _setpointPublication(robotControlSetpointTopic()),
    _paramMotorAction(this, "FS_MOT_ACT", 1),
    _paramMotorValue(this, "FS_MOT_VAL", 0.0f),
    _paramSteeringAction(this, "FS_STR_ACT", 0),
    _paramSteeringValue(this, "FS_STR_VAL", 0.0f)
{
  _failsafe.active = true;
  (void)updateParams();
  (void)refreshPolicy();
}



/** 创建目标生成对象。 */
RobotControl *RobotControl::instantiate(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  return new RobotControl();
}

/** 输出生命周期命令。 */
int RobotControl::print_usage(const char *reason)
{
  if (reason != nullptr) { fprintf(stderr, "%s\n", reason); }
  printf("usage: robot_control {start|stop|status}\n");
  return reason == nullptr ? 0 : -1;
}

/** 将目标限制在归一化范围内。 */
float RobotControl::applyAction(int32_t action, float lastValue,
                                float fixedValue)
{
  float result = action == 0 ? lastValue : (action == 2 ? fixedValue : 0.0f);
  if (!isfinite(result)) { result = 0.0f; }
  return result < -1.0f ? -1.0f : (result > 1.0f ? 1.0f : result);
}

/** Failsafe任务停止或阻塞时也按故障状态处理。 */
bool RobotControl::fresh(uint64_t timestamp, uint64_t now)
{
  return timestamp != 0 && timestamp <= now &&
         now - timestamp <= FailsafeStatusTimeoutUs;
}

/** 验证动力和转向策略编号。 */
bool RobotControl::refreshPolicy()
{
  const int32_t motorAction = _paramMotorAction.get();
  const int32_t steeringAction = _paramSteeringAction.get();
  return motorAction >= 0 && motorAction <= 2 &&
         steeringAction >= 0 && steeringAction <= 2;
}

/** 正常发布遥控目标；故障时默认油门归零并保持最后转向。 */
void RobotControl::update(uint64_t now)
{
  os::LockGuard guard(_stateMutex);
  if (!guard.locked()) { return; }
  if (now == 0) { now = os::Clock::nowMicroseconds(); }

  ParameterUpdate parameterUpdate{};
  if (_parameterSubscription.update(parameterUpdate))
    {
      (void)updateParams();
      (void)refreshPolicy();
    }

  ManualControl received{};
  if (_manualSubscription.update(received)) { _manual = received; }
  FailsafeStatus failsafe{};
  if (_failsafeSubscription.update(failsafe))
    {
      _failsafe = failsafe;
      _failsafeSeen = true;
    }

  if (_manual.valid && _manual.timestampSample != 0 &&
      _manual.timestampSample <= now)
    {
      _lastValidManual = _manual;
      _haveValidInput = true;
    }

  const bool failsafeActive = !_failsafeSeen ||
                              !fresh(_failsafe.timestamp, now) ||
                              _failsafe.active;
  _setpoint.timestamp = now;
  _setpoint.failsafe = failsafeActive;

  if (!_haveValidInput)
    {
      _setpoint.speed = 0.0f;
      _setpoint.steering = 0.0f;
      _setpoint.timestampSample = 0;
      _setpoint.valid = false;
    }
  else
    {
      _setpoint.timestampSample = _lastValidManual.timestampSample;
      _setpoint.speed = failsafeActive
          ? applyAction(_paramMotorAction.get(), _lastValidManual.throttle,
                        _paramMotorValue.get())
          : _lastValidManual.throttle;
      _setpoint.steering = failsafeActive
          ? applyAction(_paramSteeringAction.get(), _lastValidManual.yaw,
                        _paramSteeringValue.get())
          : _lastValidManual.yaw;
      _setpoint.valid = true;
    }

  (void)_setpointPublication.publish(_setpoint);
}

/** 持锁输出目标、Failsafe状态及当前策略。 */
int RobotControl::print_status()
{
  RobotControlSetpoint setpoint{};
  int32_t motorAction;
  int32_t steeringAction;
  {
    os::LockGuard guard(_stateMutex);
    if (!guard.locked()) { return -1; }
    setpoint = _setpoint;
    motorAction = _paramMotorAction.get();
    steeringAction = _paramSteeringAction.get();
  }
  printf("running\nfailsafe active: %s\nsetpoint valid: %s\n",
         setpoint.failsafe ? "yes" : "no", setpoint.valid ? "yes" : "no");
  printf("speed/steering: %.3f %.3f\n", static_cast<double>(setpoint.speed),
         static_cast<double>(setpoint.steering));
  printf("motor action: %ld, steering action: %ld\n",
         static_cast<long>(motorAction), static_cast<long>(steeringAction));
  return 0;
}

/** 周期发布目标；停止后立即发布无效的安全目标。 */
bool RobotControl::initWorkQueue()
{
  return _manualSubscription.registerCallback() &&
         _failsafeSubscription.registerCallback() &&
         _parameterSubscription.registerCallback();
}

void RobotControl::unregisterCallbacks()
{
  _manualSubscription.unregisterCallback();
  _failsafeSubscription.unregisterCallback();
  _parameterSubscription.unregisterCallback();
}

void RobotControl::Run()
{
  update();
}

void RobotControl::onWorkQueueStopped()
{
  RobotControlSetpoint stopped{};
  stopped.timestamp = os::Clock::nowMicroseconds();
  stopped.failsafe = true;
  (void)_setpointPublication.publish(stopped);
}

/** 提供NSH的robot_control命令入口。 */
extern "C" int robot_control_main(int argc, char *argv[])
{
  return RobotControl::main(argc, argv);
}
