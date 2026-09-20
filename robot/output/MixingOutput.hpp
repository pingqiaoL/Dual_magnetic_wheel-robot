/** @file MixingOutput.hpp
 * @brief 统一订阅执行器和安全状态，通过虚函数回调持有它的输出驱动。
 */
#pragma once
#include "robot/output/OutputInterface.hpp"
#include "robot/orb/Topics.hpp"

/** CAN/PWM/UART驱动可复用的公共输出处理器，不创建自己的线程。 */
class MixingOutput
{
public:
  /** 引用外部驱动接口；接口的生命周期必须覆盖本对象。 */
  MixingOutput(OutputInterface &interface, uint8_t maxMotors, uint8_t maxServos);
  /** 执行公共处理并回调驱动；默认在读取输入后采时，测试可指定时间。 */
  bool update(uint64_t now = 0);

private:
  /** 验证时间戳未过期且不处于未来。 */
  static bool fresh(uint64_t timestamp, uint64_t now);
  /** 检查数量、有限数值与消息有效性。 */
  static bool validOutputs(const float *outputs, uint8_t count,
                           uint8_t maximum, bool valid);
  /** 将输出限制到归一化范围。 */
  static float constrainUnit(float value);
  /** 向两类执行器发送停止回调，失败时下个周期继续重试。 */
  bool stopOutputs(uint64_t now);

  OutputInterface &_interface;
  uorb::Subscription<ActuatorArmed> _armedSubscription;
  uorb::Subscription<ActuatorMotors> _motorsSubscription;
  uorb::Subscription<ActuatorServos> _servosSubscription;
  ActuatorArmed _armed{};
  ActuatorMotors _motors{};
  ActuatorServos _servos{};
  const uint8_t _maxMotors;
  const uint8_t _maxServos;
  uint64_t _lastOutput{0};
  bool _stopped{true};
};
