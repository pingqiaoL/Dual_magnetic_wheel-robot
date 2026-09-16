/**
 * @file RobotConfiguration.hpp
 * @brief 定义机器人构型父类，只描述执行器数量和可逆动力电机。
 */

#pragma once

#include <stdint.h>

/** 所有机器人机型必须实现的最小构型接口。 */
class RobotConfiguration
{
public:
  virtual ~RobotConfiguration() = default;

  /** 返回供 NSH 和日志显示的机型名称。 */
  virtual const char *name() const = 0;

  /** 返回 actuator_motors 中动力电机数量。 */
  virtual uint8_t motorCount() const = 0;

  /** 返回 actuator_servos 中转向执行器数量。 */
  virtual uint8_t servoCount() const = 0;

  /** 每一位表示对应动力电机是否允许负输出。 */
  virtual uint32_t reversibleMotorMask() const = 0;
};
