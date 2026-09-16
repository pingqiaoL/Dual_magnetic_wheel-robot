/**
 * @file ClimbotAllocation.hpp
 * @brief 声明 Climbot 前后动力和前后转向的参数化分配算法。
 */

#pragma once

#include "robot/control/Allocation.hpp"

/** 使用 CA_THR_* 和 CA_YAW_* 参数完成 Climbot 分配。 */
class ClimbotAllocation final : public Allocation
{
public:
  ClimbotAllocation();
  bool updateParameters() override;
  void allocate(const ManualControl &manual, bool armed,
                ActuatorMotors &motors,
                ActuatorServos &servos) const override;

private:
  float _throttleFront;
  float _throttleRear;
  float _yawFront;
  float _yawRear;
};
