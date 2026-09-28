/** @file ActuatorEffectivenessDualMagneticWheel.cpp
 * @brief 将双磁轮构型参数转换为通用分配算法使用的四行矩阵。
 */
#include "robot/control/ActuatorEffectivenessDualMagneticWheel.hpp"
#include "robot/params/ParamManager.hpp"
#include <math.h>

ActuatorEffectivenessDualMagneticWheel::ActuatorEffectivenessDualMagneticWheel()
    : ActuatorEffectiveness(nullptr),
      _paramThrottleFront(this, "CA_THR_F", 1.0f),
      _paramThrottleRear(this, "CA_THR_R", 1.0f),
      _paramYawFront(this, "CA_YAW_F", 1.0f),
      _paramYawRear(this, "CA_YAW_R", 0.0f)
{
  (void)updateParams();
}

/** 默认矩阵为[1,0]、[1,0]、[0,1]、[0,0]，后转向保持中位。 */
bool ActuatorEffectivenessDualMagneticWheel::getAllocationMatrix(
    AllocationMatrix &matrix) const
{
  matrix = AllocationMatrix{};
  matrix.values[0][0] = _paramThrottleFront.get();
  matrix.values[1][0] = _paramThrottleRear.get();
  matrix.values[2][1] = _paramYawFront.get();
  matrix.values[3][1] = _paramYawRear.get();
  const bool valid = true;
  for (uint8_t row = 0; row < motorCount() + servoCount(); ++row)
    {
      if (!isfinite(matrix.values[row][0]) || !isfinite(matrix.values[row][1]))
        { return false; }
    }
  return valid;
}
