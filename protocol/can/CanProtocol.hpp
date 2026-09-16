/**
 * @file CanProtocol.hpp
 * @brief 定义与 NuttX 无关的 CAN 帧和可选电机协议编号。
 */

#pragma once

#include <stdint.h>

/** 参数 CAN_*_PROTO 使用的稳定编号。 */
enum class CanProtocol : int32_t
{
  Disabled = 0,
  Damiao = 1
};

/** 协议编解码层与具体 CAN 驱动交换的标准帧。 */
struct CanFrame
{
  uint32_t id{0};
  uint8_t length{0};
  uint8_t data[8]{};
};
