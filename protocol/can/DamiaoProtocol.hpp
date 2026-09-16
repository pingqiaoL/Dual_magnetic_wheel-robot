/**
 * @file DamiaoProtocol.hpp
 * @brief 声明达妙通用 CAN 协议的控制帧编码和反馈帧解析。
 */

#pragma once

#include "protocol/can/CanProtocol.hpp"

/** 达妙上位机工具中的电机型号编号。 */
enum class DamiaoMotorType : int32_t
{
  Unknown = 0,
  DM4310 = 1,
  DMS3519 = 2
};

/** 达妙通用协议的四种控制模式编号。 */
enum class DamiaoMode : int32_t
{
  Mit = 1,
  PositionVelocity = 2,
  Velocity = 3,
  PositionVelocityTorque = 4
};

/** 电机协议量化和反馈解析所需的物理范围。 */
struct DamiaoLimits
{
  float positionMax{12.5f};
  float velocityMax{30.0f};
  float torqueMax{10.0f};
};

/** 从达妙反馈帧恢复的物理量。 */
struct DamiaoFeedback
{
  uint8_t motorId{0};
  uint8_t error{0};
  float position{0.0f};
  float velocity{0.0f};
  float torque{0.0f};
  float mosTemperature{0.0f};
  float coilTemperature{0.0f};
};

/** 纯编解码类，不访问设备、不创建线程，也不使用 uORB。 */
class DamiaoProtocol
{
public:
  /** 生成使能帧，末字节为 0xfc。 */
  static bool encodeEnable(uint16_t motorId, DamiaoMode mode,
                           CanFrame &frame);

  /** 生成失能帧，末字节为 0xfd。 */
  static bool encodeDisable(uint16_t motorId, DamiaoMode mode,
                            CanFrame &frame);

  /** 生成保存当前位置为零点的帧，末字节为 0xfe。 */
  static bool encodeSetZero(uint16_t motorId, DamiaoMode mode,
                            CanFrame &frame);

  /** 生成清除故障帧，末字节为 0xfb。 */
  static bool encodeClearError(uint16_t motorId, DamiaoMode mode,
                               CanFrame &frame);

  /** 生成速度模式帧：CAN ID 为 0x200 + motorId。 */
  static bool encodeVelocity(uint16_t motorId, float velocity,
                             CanFrame &frame);

  /** 生成位置速度模式帧：CAN ID 为 0x100 + motorId。 */
  static bool encodePositionVelocity(uint16_t motorId, float position,
                                     float velocity, CanFrame &frame);

  /** 解析达妙 8 字节状态反馈。 */
  static bool decodeFeedback(const CanFrame &frame,
                             const DamiaoLimits &limits,
                             DamiaoFeedback &feedback);

  /** 返回控制模式对应的 CAN ID 偏移。 */
  static bool modeOffset(DamiaoMode mode, uint16_t &offset);

private:
  static bool encodeSpecial(uint16_t motorId, DamiaoMode mode,
                            uint8_t command, CanFrame &frame);
  static void encodeFloatLittleEndian(float value, uint8_t *destination);
  static float unsignedToFloat(uint32_t value, float minimum,
                               float maximum, unsigned bits);
};
