/**
 * @file Topics.cpp
 * @brief 为遥控链路的每个消息类型创建唯一的全局 topic 实例。
 */

#include "robot/orb/Topics.hpp"

/** 返回进程内唯一的 input_rc topic。 */
uorb::Topic<InputRc> &inputRcTopic()
{
  static uorb::Topic<InputRc> topic;
  return topic;
}

/** 返回进程内唯一的 rc_channels topic。 */
uorb::Topic<RcChannels> &rcChannelsTopic()
{
  static uorb::Topic<RcChannels> topic;
  return topic;
}

/** 返回进程内唯一的 manual_control_input topic。 */
uorb::Topic<ManualControl> &manualControlTopic()
{
  static uorb::Topic<ManualControl> topic;
  return topic;
}

/** 返回进程内唯一的 manual_control_switches topic。 */
uorb::Topic<ManualControlSwitches> &manualControlSwitchesTopic()
{
  static uorb::Topic<ManualControlSwitches> topic;
  return topic;
}

/** 返回进程内唯一的 parameter_update topic。 */
uorb::Topic<ParameterUpdate> &parameterUpdateTopic()
{
  static uorb::Topic<ParameterUpdate> topic;
  return topic;
}

/** 返回进程内唯一的 actuator_motors topic。 */
uorb::Topic<ActuatorMotors> &actuatorMotorsTopic()
{
  static uorb::Topic<ActuatorMotors> topic;
  return topic;
}

/** 返回进程内唯一的 actuator_servos topic。 */
uorb::Topic<ActuatorServos> &actuatorServosTopic()
{
  static uorb::Topic<ActuatorServos> topic;
  return topic;
}

/** 返回进程内唯一的 actuator_status topic。 */
uorb::Topic<ActuatorStatus> &actuatorStatusTopic()
{
  static uorb::Topic<ActuatorStatus> topic;
  return topic;
}

/** 返回进程内唯一的actuator_armed topic。 */
uorb::Topic<ActuatorArmed> &actuatorArmedTopic()
{
  static uorb::Topic<ActuatorArmed> topic;
  return topic;
}
