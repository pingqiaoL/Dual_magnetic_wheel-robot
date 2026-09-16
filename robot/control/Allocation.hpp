/**
 * @file Allocation.hpp
 * @brief 定义从手动控制量到电机和舵机输出的分配算法接口。
 */

#pragma once

#include "msg/ActuatorMotors.hpp"
#include "msg/ActuatorServos.hpp"
#include "msg/ManualControl.hpp"

/** 可由不同机器人构型重写的动力分配父类。 */
class Allocation
{
public:
  virtual ~Allocation() = default;

  /** 刷新分配系数参数。 */
  virtual bool updateParameters() = 0;

  /** 把已校准的手动输入分配为两类执行器输出。 */
  virtual void allocate(const ManualControl &manual, bool armed,
                        ActuatorMotors &motors,
                        ActuatorServos &servos) const = 0;
};
