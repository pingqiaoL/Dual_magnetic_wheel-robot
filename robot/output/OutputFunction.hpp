/**
 * @file OutputFunction.hpp
 * @brief 定义与物理总线无关的逻辑执行器功能编号。
 */

#pragma once

#include <stdint.h>

/** 数值布局与PX4的Motor/Servo输出功能分段保持一致。 */
enum class OutputFunction : uint16_t
{
  Disabled = 0,
  Motor1 = 101,
  Motor2 = 102,
  MotorMax = 108,
  Servo1 = 201,
  Servo2 = 202,
  ServoMax = 208
};

inline bool outputFunctionIsMotor(OutputFunction function)
{
  const uint16_t value = static_cast<uint16_t>(function);
  return value >= static_cast<uint16_t>(OutputFunction::Motor1) &&
         value <= static_cast<uint16_t>(OutputFunction::MotorMax);
}

inline bool outputFunctionIsServo(OutputFunction function)
{
  const uint16_t value = static_cast<uint16_t>(function);
  return value >= static_cast<uint16_t>(OutputFunction::Servo1) &&
         value <= static_cast<uint16_t>(OutputFunction::ServoMax);
}

inline uint8_t outputFunctionIndex(OutputFunction function)
{
  return static_cast<uint8_t>(static_cast<uint16_t>(function) -
      (outputFunctionIsMotor(function)
          ? static_cast<uint16_t>(OutputFunction::Motor1)
          : static_cast<uint16_t>(OutputFunction::Servo1)));
}

inline const char *outputFunctionName(OutputFunction function)
{
  switch (function)
    {
      case OutputFunction::Disabled: return "disabled";
      case OutputFunction::Motor1: return "motor1";
      case OutputFunction::Motor2: return "motor2";
      case OutputFunction::Servo1: return "servo1";
      case OutputFunction::Servo2: return "servo2";
      default: return "unsupported";
    }
}
