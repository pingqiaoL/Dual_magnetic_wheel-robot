/**
 * @file DamiaoProtocol.cpp
 * @brief 按达妙官方 F4 示例实现通用控制协议，不依赖 STM32 HAL。
 */

#include "protocol/can/DamiaoProtocol.hpp"

#include <math.h>
#include <string.h>

/** 将协议模式转换为官方规定的标准帧 ID 偏移。 */
bool DamiaoProtocol::modeOffset(DamiaoMode mode, uint16_t &offset)
{
  switch (mode)
    {
      case DamiaoMode::Mit:
        offset = 0x000;
        return true;
      case DamiaoMode::PositionVelocity:
        offset = 0x100;
        return true;
      case DamiaoMode::Velocity:
        offset = 0x200;
        return true;
      case DamiaoMode::PositionVelocityTorque:
        offset = 0x300;
        return true;
      default:
        return false;
    }
}

bool DamiaoProtocol::encodeEnable(uint16_t motorId, DamiaoMode mode,
                                  CanFrame &frame)
{
  return encodeSpecial(motorId, mode, 0xfc, frame);
}

bool DamiaoProtocol::encodeDisable(uint16_t motorId, DamiaoMode mode,
                                   CanFrame &frame)
{
  return encodeSpecial(motorId, mode, 0xfd, frame);
}

bool DamiaoProtocol::encodeSetZero(uint16_t motorId, DamiaoMode mode,
                                   CanFrame &frame)
{
  return encodeSpecial(motorId, mode, 0xfe, frame);
}

bool DamiaoProtocol::encodeClearError(uint16_t motorId, DamiaoMode mode,
                                      CanFrame &frame)
{
  return encodeSpecial(motorId, mode, 0xfb, frame);
}

/** 生成七个 0xff 加一个功能码的电机管理帧。 */
bool DamiaoProtocol::encodeSpecial(uint16_t motorId, DamiaoMode mode,
                                   uint8_t command, CanFrame &frame)
{
  uint16_t offset = 0;
  if (motorId > 0x7ffU || !modeOffset(mode, offset) ||
      motorId + offset > 0x7ffU)
    {
      return false;
    }

  frame = CanFrame{};
  frame.id = static_cast<uint32_t>(motorId + offset);
  frame.length = 8;
  memset(frame.data, 0xff, 7);
  frame.data[7] = command;
  return true;
}

/** 速度模式只发送一个 IEEE754 单精度数，字节序与 STM32 小端一致。 */
bool DamiaoProtocol::encodeVelocity(uint16_t motorId, float velocity,
                                    CanFrame &frame)
{
  if (motorId + 0x200U > 0x7ffU || !isfinite(velocity))
    {
      return false;
    }

  frame = CanFrame{};
  frame.id = static_cast<uint32_t>(motorId + 0x200U);
  frame.length = 4;
  encodeFloatLittleEndian(velocity, frame.data);
  return true;
}

/** 位置速度模式依次发送小端 float 位置和小端 float 速度。 */
bool DamiaoProtocol::encodePositionVelocity(uint16_t motorId, float position,
                                            float velocity, CanFrame &frame)
{
  if (motorId + 0x100U > 0x7ffU || !isfinite(position) ||
      !isfinite(velocity))
    {
      return false;
    }

  frame = CanFrame{};
  frame.id = static_cast<uint32_t>(motorId + 0x100U);
  frame.length = 8;
  encodeFloatLittleEndian(position, &frame.data[0]);
  encodeFloatLittleEndian(velocity, &frame.data[4]);
  return true;
}

/** 解析官方 16 位位置、12 位速度、12 位转矩和两路温度布局。 */
bool DamiaoProtocol::decodeFeedback(const CanFrame &frame,
                                    const DamiaoLimits &limits,
                                    DamiaoFeedback &feedback)
{
  if (frame.length != 8 || !(limits.positionMax > 0.0f) ||
      !(limits.velocityMax > 0.0f) || !(limits.torqueMax > 0.0f))
    {
      return false;
    }

  const uint32_t position =
      (static_cast<uint32_t>(frame.data[1]) << 8) | frame.data[2];
  const uint32_t velocity =
      (static_cast<uint32_t>(frame.data[3]) << 4) | (frame.data[4] >> 4);
  const uint32_t torque =
      (static_cast<uint32_t>(frame.data[4] & 0x0fU) << 8) | frame.data[5];

  feedback.motorId = frame.data[0] & 0x0fU;
  feedback.error = frame.data[0] >> 4;
  feedback.position = unsignedToFloat(position, -limits.positionMax,
                                      limits.positionMax, 16);
  feedback.velocity = unsignedToFloat(velocity, -limits.velocityMax,
                                      limits.velocityMax, 12);
  feedback.torque = unsignedToFloat(torque, -limits.torqueMax,
                                    limits.torqueMax, 12);
  feedback.mosTemperature = static_cast<float>(frame.data[6]);
  feedback.coilTemperature = static_cast<float>(frame.data[7]);
  return true;
}

/** 明确拆分字节，避免协议层依赖主机内存字节序。 */
void DamiaoProtocol::encodeFloatLittleEndian(float value,
                                             uint8_t *destination)
{
  uint32_t raw = 0;
  memcpy(&raw, &value, sizeof(raw));
  destination[0] = static_cast<uint8_t>(raw);
  destination[1] = static_cast<uint8_t>(raw >> 8);
  destination[2] = static_cast<uint8_t>(raw >> 16);
  destination[3] = static_cast<uint8_t>(raw >> 24);
}

/** 把固定位宽无符号数线性还原到指定物理范围。 */
float DamiaoProtocol::unsignedToFloat(uint32_t value, float minimum,
                                      float maximum, unsigned bits)
{
  const uint32_t maximumInteger = (1UL << bits) - 1UL;
  return static_cast<float>(value) * (maximum - minimum) /
             static_cast<float>(maximumInteger) +
         minimum;
}
