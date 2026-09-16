/**
 * @file ClimbotAllocation.cpp
 * @brief 实现两动力轮和两转向执行器的简单线性分配。
 */

#include "robot/control/ClimbotAllocation.hpp"

#include "robot/params/ParamManager.hpp"

namespace
{
/** 把归一化输出限制在执行器 topic 约定的范围内。 */
float constrainOutput(float value)
{
  return value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
}
} // namespace

/** 使用安全初值创建分配器，并读取参数表。 */
ClimbotAllocation::ClimbotAllocation()
    : _throttleFront(1.0f),
      _throttleRear(1.0f),
      _yawFront(1.0f),
      _yawRear(0.0f)
{
  (void)updateParameters();
}

/** 从统一参数系统刷新动力和偏航系数。 */
bool ClimbotAllocation::updateParameters()
{
  ParamManager &manager = ParamManager::instance();
  bool valid = true;
  valid = manager.get("CA_THR_F", _throttleFront) && valid;
  valid = manager.get("CA_THR_R", _throttleRear) && valid;
  valid = manager.get("CA_YAW_F", _yawFront) && valid;
  valid = manager.get("CA_YAW_R", _yawRear) && valid;
  return valid;
}

/** 油门分给前后动力轮，偏航分给前后转向执行器。 */
void ClimbotAllocation::allocate(const ManualControl &manual, bool armed,
                                 ActuatorMotors &motors,
                                 ActuatorServos &servos) const
{
  motors.timestamp = manual.timestamp;
  motors.timestampSample = manual.timestampSample;
  motors.count = 2;
  motors.armed = armed;
  motors.valid = manual.valid;

  servos.timestamp = manual.timestamp;
  servos.timestampSample = manual.timestampSample;
  servos.count = 2;
  servos.armed = armed;
  servos.valid = manual.valid;

  if (!manual.valid || !armed)
    {
      return;
    }

  motors.control[0] = constrainOutput(manual.throttle * _throttleFront);
  motors.control[1] = constrainOutput(manual.throttle * _throttleRear);
  servos.control[0] = constrainOutput(manual.yaw * _yawFront);
  servos.control[1] = constrainOutput(manual.yaw * _yawRear);
}
