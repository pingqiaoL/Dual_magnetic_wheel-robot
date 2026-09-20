/** @file Allocation.cpp
 * @brief 执行构型矩阵乘法，分配器只计算输出，不解释遥控开关。
 */
#include "robot/control/Allocation.hpp"
#include <math.h>

/** 引用构型模型并生成初始矩阵。 */
Allocation::Allocation(ActuatorEffectiveness &effectiveness)
  : _effectiveness(effectiveness)
{
  (void)updateParameters();
}

/** 校验执行器数量，只有完整有效的新矩阵才替换旧矩阵。 */
bool Allocation::updateParameters()
{
  AllocationMatrix matrix{};
  _valid = _effectiveness.motorCount() <= ActuatorMotors::MAX_CONTROLS &&
           _effectiveness.servoCount() <= ActuatorServos::MAX_CONTROLS &&
           _effectiveness.getAllocationMatrix(matrix);
  if (_valid) { _matrix = matrix; }
  return _valid;
}

/** 共用归一化限幅，避免参数系数放大后超过输出范围。 */
float Allocation::constrainUnit(float value)
{
  return value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
}

/** a=M*[throttle,yaw]；有效输出即使尚未解锁也发布，由MixingOutput把关。 */
void Allocation::allocate(const ManualControl &manual,
                           ActuatorMotors &motors, ActuatorServos &servos) const
{
  motors = ActuatorMotors{};
  servos = ActuatorServos{};
  motors.timestamp = servos.timestamp = manual.timestamp;
  motors.timestampSample = servos.timestampSample = manual.timestampSample;
  motors.count = _effectiveness.motorCount();
  servos.count = _effectiveness.servoCount();
  motors.valid = servos.valid = _valid && manual.valid &&
                               isfinite(manual.throttle) && isfinite(manual.yaw);
  if (!motors.valid) { return; }
  for (uint8_t row = 0; row < motors.count + servos.count; ++row)
    {
      const float result = _matrix.values[row][0] * manual.throttle +
                           _matrix.values[row][1] * manual.yaw;
      if (!isfinite(result))
        {
          motors = ActuatorMotors{};
          servos = ActuatorServos{};
          return;
        }
      float value = constrainUnit(result);
      if (row < motors.count)
        {
          if ((_effectiveness.reversibleMotorMask() & (1U << row)) == 0 &&
              value < 0.0f) { value = 0.0f; }
          motors.control[row] = value;
        }
      else { servos.control[row - motors.count] = value; }
    }
}
