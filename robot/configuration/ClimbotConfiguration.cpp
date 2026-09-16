/**
 * @file ClimbotConfiguration.cpp
 * @brief 实现第一版 Climbot 的执行器统计。
 */

#include "robot/configuration/ClimbotConfiguration.hpp"

/** 返回构型选择名称。 */
const char *ClimbotConfiguration::name() const
{
  return "climbot";
}

/** 前后两个 S3519 均属于动力电机。 */
uint8_t ClimbotConfiguration::motorCount() const
{
  return 2;
}

/** 前后两个 4310 均作为转向执行器。 */
uint8_t ClimbotConfiguration::servoCount() const
{
  return 2;
}

/** 两个动力轮都允许正反转。 */
uint32_t ClimbotConfiguration::reversibleMotorMask() const
{
  return 0x03U;
}
