/**
 * @file OutputInterface.hpp
 * @brief 定义 PWM、CAN、UART 等执行器输出驱动共同继承的接口。
 */

#pragma once

#include <stdint.h>

/** 区分 PX4 风格的动力电机输出和舵机/转向输出。 */
enum class ActuatorType : uint8_t
{
  Motor = 0,
  Servo = 1
};

/** 各种物理输出驱动必须实现的父类。 */
class OutputInterface
{
public:
  virtual ~OutputInterface() = default;

  /** 打开并初始化底层输出设备。 */
  virtual bool init() = 0;

  /** 把一组归一化输出写到相应物理执行器。 */
  virtual bool updateOutputs(ActuatorType type, bool stopMotors,
                             const float *outputs, uint8_t count,
                             uint64_t now) = 0;
};
