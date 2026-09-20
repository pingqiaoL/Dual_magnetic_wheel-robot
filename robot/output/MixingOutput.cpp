/** @file MixingOutput.cpp
 * @brief 处理各输出驱动共有的解锁状态、数据超时、数值检查和限幅。
 */
#include "robot/output/MixingOutput.hpp"
#include "robot/os/Clock.hpp"
#include <math.h>

/** 将外部驱动的*this保存为引用，绑定三个独立的uORB订阅。 */
MixingOutput::MixingOutput(OutputInterface &interface, uint8_t maxMotors,
                           uint8_t maxServos)
  : _interface(interface), _armedSubscription(actuatorArmedTopic()),
    _motorsSubscription(actuatorMotorsTopic()),
    _servosSubscription(actuatorServosTopic()),
    _maxMotors(maxMotors), _maxServos(maxServos)
{
}

/** 共用500毫秒超时，避免任务停止后继续执行缓存目标。 */
bool MixingOutput::fresh(uint64_t timestamp, uint64_t now)
{
  return timestamp != 0 && timestamp <= now && now - timestamp <= 500000ULL;
}

/** 数量不得超过消息和驱动容量，NaN/Inf不得进入任何物理驱动。 */
bool MixingOutput::validOutputs(const float *outputs, uint8_t count,
                                uint8_t maximum, bool valid)
{
  if (!valid || count > maximum || count > ActuatorMotors::MAX_CONTROLS)
    { return false; }
  for (uint8_t index = 0; index < count; ++index)
    { if (!isfinite(outputs[index])) { return false; } }
  return true;
}

/** 在协议编码前完成所有驱动共用的归一化限幅。 */
float MixingOutput::constrainUnit(float value)
{
  return value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
}

/** stopMotors也用于转向执行器失能；不使用零值代替实际失能。 */
bool MixingOutput::stopOutputs(uint64_t now)
{
  const bool motors = _interface.updateOutputs(ActuatorType::Motor, true,
                                               nullptr, 0, now);
  const bool servos = _interface.updateOutputs(ActuatorType::Servo, true,
                                               nullptr, 0, now);
  _stopped = true;
  _lastOutput = now;
  return motors && servos;
}

/** 读取最新消息，执行公共处理，再经虚函数调用真实驱动。 */
bool MixingOutput::update(uint64_t now)
{
  const bool armChanged = _armedSubscription.update(_armed);
  const bool motorsChanged = _motorsSubscription.update(_motors);
  const bool servosChanged = _servosSubscription.update(_servos);
  // 在完成消息快照之后采时，避免把高优先级任务刚发布的消息误判为未来。
  if (now == 0) { now = os::Clock::nowMicroseconds(); }
  const bool stop = !_armed.valid || !_armed.armed || _armed.lockdown ||
      !fresh(_armed.timestamp, now) || !fresh(_motors.timestamp, now) ||
      !fresh(_servos.timestamp, now) ||
      !fresh(_motors.timestampSample, now) ||
      !fresh(_servos.timestampSample, now) ||
      !validOutputs(_motors.control, _motors.count, _maxMotors, _motors.valid) ||
      !validOutputs(_servos.control, _servos.count, _maxServos, _servos.valid) ||
      (_motors.count == 0 && _servos.count == 0);

  if (!armChanged && !motorsChanged && !servosChanged && stop == _stopped &&
      _lastOutput != 0 && now - _lastOutput < 20000ULL)
    { return true; }
  if (stop) { return stopOutputs(now); }

  float motors[ActuatorMotors::MAX_CONTROLS]{};
  float servos[ActuatorServos::MAX_CONTROLS]{};
  for (uint8_t index = 0; index < _motors.count; ++index)
    { motors[index] = constrainUnit(_motors.control[index]); }
  for (uint8_t index = 0; index < _servos.count; ++index)
    { servos[index] = constrainUnit(_servos.control[index]); }

  if (!_interface.updateOutputs(ActuatorType::Motor, false,
                                motors, _motors.count, now) ||
      !_interface.updateOutputs(ActuatorType::Servo, false,
                                servos, _servos.count, now))
    {
      (void)stopOutputs(now);
      return false;
    }
  _stopped = false;
  _lastOutput = now;
  return true;
}
