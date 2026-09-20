/**
 * @file Allocation.hpp
 * @brief 定义从手动控制量到电机和舵机输出的分配算法接口。
 */

#pragma once

#include "msg/ActuatorMotors.hpp"
#include "msg/ActuatorServos.hpp"
#include "msg/ManualControl.hpp"
#include "robot/control/ActuatorEffectiveness.hpp"

/** 通用分配算法；机型变化通过ActuatorEffectiveness子类描述。 */
class Allocation
{
public:
  /** 绑定构型模型，模型必须比分配算法活得更久。 */
  explicit Allocation(ActuatorEffectiveness &effectiveness);
  /** 允许后续派生新的控制分配算法。 */
  virtual ~Allocation() = default;

  /** 刷新分配系数参数。 */
  virtual bool updateParameters();

  /** 把已校准的手动输入分配为两类执行器输出。 */
  virtual void allocate(const ManualControl &manual,
                        ActuatorMotors &motors,
                        ActuatorServos &servos) const;
private:
  /** 执行归一化范围限幅。 */
  static float constrainUnit(float value);
  ActuatorEffectiveness &_effectiveness;
  AllocationMatrix _matrix{};
  bool _valid{false};
};
