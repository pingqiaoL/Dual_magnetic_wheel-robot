/**
 * @file Topics.hpp
 * @brief 声明遥控、参数和执行器链路使用的全局 uORB topic。
 */

#pragma once

#include "msg/ActuatorArmed.hpp"
#include "msg/ActuatorMotors.hpp"
#include "msg/ActuatorServos.hpp"
#include "msg/ActuatorStatus.hpp"
#include "msg/InputRc.hpp"
#include "msg/ManualControl.hpp"
#include "msg/ManualControlSwitches.hpp"
#include "msg/ParameterUpdate.hpp"
#include "msg/RcChannels.hpp"
#include "robot/orb/uORB.hpp"

/** 返回原始遥控输入 topic：input_rc。 */
uorb::Topic<InputRc> &inputRcTopic();

/** 返回归一化遥控通道 topic：rc_channels。 */
uorb::Topic<RcChannels> &rcChannelsTopic();

/** 返回摇杆控制 topic：manual_control_input。 */
uorb::Topic<ManualControl> &manualControlTopic();

/** 返回遥控开关 topic：manual_control_switches。 */
uorb::Topic<ManualControlSwitches> &manualControlSwitchesTopic();

/** 返回参数变化通知 topic：parameter_update。 */
uorb::Topic<ParameterUpdate> &parameterUpdateTopic();

/** 返回动力电机输出 topic：actuator_motors。 */
uorb::Topic<ActuatorMotors> &actuatorMotorsTopic();

/** 返回转向执行器输出 topic：actuator_servos。 */
uorb::Topic<ActuatorServos> &actuatorServosTopic();

/** 返回 CAN 执行器反馈 topic：actuator_status。 */
uorb::Topic<ActuatorStatus> &actuatorStatusTopic();

/** 返回command发布的统一执行器安全状态topic。 */
uorb::Topic<ActuatorArmed> &actuatorArmedTopic();
