/** @file ActuatorEffectiveness.hpp
 * @brief 定义可按参数创建的构型模型与轮式机器人的分配矩阵接口。
 */
#pragma once
#include "robot/configuration/RobotConfiguration.hpp"
#include "msg/ActuatorMotors.hpp"
#include "msg/ActuatorServos.hpp"

/** 每行对应一个执行器，两列依次对应前进输入和偏航输入。 */
struct AllocationMatrix
{
  static constexpr uint8_t MaxActuators = ActuatorMotors::MAX_CONTROLS +
                                         ActuatorServos::MAX_CONTROLS;
  float values[MaxActuators][2]{};
};

/** 构型模型父类，继承执行器统计接口，让分配任务不依赖具体机型。 */
class ActuatorEffectiveness : public RobotConfiguration
{
public:
  /** 通过父类指针安全释放构型子类。 */
  ~ActuatorEffectiveness() override = default;
  /** 根据构型参数生成直接分配矩阵；行顺序为电机后舵机。
   * 当前轮式控制使用直接线性映射，不是飞行器力/力矩矩阵的伪逆。
   */
  virtual bool getAllocationMatrix(AllocationMatrix &matrix) const = 0;
};
