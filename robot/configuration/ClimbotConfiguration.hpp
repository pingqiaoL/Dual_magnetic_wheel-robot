/**
 * @file ClimbotConfiguration.hpp
 * @brief 声明两动力轮、两转向执行器的 Climbot 构型。
 */

#pragma once

#include "robot/configuration/RobotConfiguration.hpp"

/** 第一版磁轮爬壁机器人构型。 */
class ClimbotConfiguration final : public RobotConfiguration
{
public:
  const char *name() const override;
  uint8_t motorCount() const override;
  uint8_t servoCount() const override;
  uint32_t reversibleMotorMask() const override;
};
