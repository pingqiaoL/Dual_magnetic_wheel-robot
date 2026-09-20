/** @file ActuatorEffectivenessDualMagneticWheel.hpp
 * @brief 描述双磁轮构型的两路动力、两路转向以及参数化分配矩阵。
 */
#pragma once
#include "robot/control/ActuatorEffectiveness.hpp"

/** CA_AIRFRAME=1对应的双磁轮构型模型。 */
class ActuatorEffectivenessDualMagneticWheel final : public ActuatorEffectiveness
{
public:
  /** 返回用于状态输出的机型名称。 */
  const char *name() const override { return "climbot"; }
  /** 前后轮各一路速度执行器。 */
  uint8_t motorCount() const override { return 2; }
  /** 前后轮各一路转向位置执行器。 */
  uint8_t servoCount() const override { return 2; }
  /** 两路动力轮都允许倒车。 */
  uint32_t reversibleMotorMask() const override { return 3; }
  /** 从CA_THR_F/R和CA_YAW_F/R生成分配矩阵。 */
  bool getAllocationMatrix(AllocationMatrix &matrix) const override;
};
