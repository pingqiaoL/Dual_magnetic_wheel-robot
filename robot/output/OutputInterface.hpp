/**
 * @file OutputInterface.hpp
 * @brief 定义 PWM、CAN、UART 等执行器输出驱动共同继承的接口。
 */

#pragma once

#include <stdint.h>

/** 动力电机输出和舵机/转向输出。 */
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

  /** 由MixingOutput回调一次完整物理通道数组。
   * stopOutputs为true时outputs可为nullptr且count为0，驱动必须执行真实失能。
   * 正常输出按物理通道排列且范围为-1到1。
   */
  virtual bool updateOutputs(bool stopOutputs, const float *outputs,
                             uint8_t count,
                             uint64_t now) = 0;
};
