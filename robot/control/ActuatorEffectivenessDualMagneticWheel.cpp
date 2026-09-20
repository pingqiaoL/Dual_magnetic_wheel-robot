/** @file ActuatorEffectivenessDualMagneticWheel.cpp
 * @brief 将双磁轮构型参数转换为通用分配算法使用的四行矩阵。
 */
#include "robot/control/ActuatorEffectivenessDualMagneticWheel.hpp"
#include "robot/params/ParamManager.hpp"
#include <math.h>

/** 默认矩阵为[1,0]、[1,0]、[0,1]、[0,0]，后转向保持中位。 */
bool ActuatorEffectivenessDualMagneticWheel::getAllocationMatrix(
    AllocationMatrix &matrix) const
{
  matrix = AllocationMatrix{};
  ParamManager &params = ParamManager::instance();
  const bool valid = params.get("CA_THR_F", matrix.values[0][0]) &&
                     params.get("CA_THR_R", matrix.values[1][0]) &&
                     params.get("CA_YAW_F", matrix.values[2][1]) &&
                     params.get("CA_YAW_R", matrix.values[3][1]);
  for (uint8_t row = 0; row < motorCount() + servoCount(); ++row)
    {
      if (!isfinite(matrix.values[row][0]) || !isfinite(matrix.values[row][1]))
        { return false; }
    }
  return valid;
}
