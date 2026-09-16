/**
 * @file sbus.cpp
 * @brief 实现 MK32 接收机标准 SBUS 帧同步、通道解包和失控标志解析。
 */

#include "robot/drivers/rc/sbus.h"

#include <string.h>

namespace
{
constexpr uint8_t SbusStartByte = 0x0f;
constexpr size_t SbusFlagsIndex = 23;
constexpr uint8_t SbusFrameLostMask = 1U << 2;
constexpr uint8_t SbusFailsafeMask = 1U << 3;
/** 判断 SBUS1/SBUS2 帧尾字节是否有效。 */
bool validEndByte(uint8_t value)
{
  return value == 0x00 || value == 0x04 || value == 0x14 ||
         value == 0x24 || value == 0x34;
}

/** 将 SBUS 的 0 到 2047 原始值换算为约 875 到 2154 的 PWM 表示。 */
uint16_t sbusRawToPwm(uint16_t rawValue)
{
  return static_cast<uint16_t>((rawValue * 5U + 4U) / 8U + 875U);
}
} // namespace

/** 初始化 SBUS 解析器。 */
SbusDecoder::SbusDecoder()
{
  reset();
}

/** 清空解析缓冲区和帧统计。 */
void SbusDecoder::reset()
{
  memset(_buffer, 0, sizeof(_buffer));
  _count = 0;
  _totalFrames = 0;
  _invalidFrames = 0;
  _receiverLostFrames = 0;
}

/** 接收一个串口字节，并在收齐 25 字节后尝试解码。 */
bool SbusDecoder::feed(uint8_t byte, uint64_t timestampMicroseconds,
                       SbusFrame &frame)
{
  // NuttX 串口驱动先把字节放入环形缓冲区，调用本函数的时间不是
  // 字节实际到达 UART 的时间，因此不能用任务读取间隔判断线上的帧间隔。
  (void)timestampMicroseconds;

  if (_count == 0)
    {
      if (byte != SbusStartByte)
        {
          return false;
        }

      _buffer[_count++] = byte;
      return false;
    }

  _buffer[_count++] = byte;
  if (_count < FrameSize)
    {
      return false;
    }

  if (!decode(frame))
    {
      __atomic_add_fetch(&_invalidFrames, 1U, __ATOMIC_RELAXED);
      recoverAfterInvalidFrame();
      return false;
    }

  _count = 0;
  __atomic_add_fetch(&_totalFrames, 1U, __ATOMIC_RELAXED);
  if (frame.frameLost)
    {
      __atomic_add_fetch(&_receiverLostFrames, 1U, __ATOMIC_RELAXED);
    }

  return true;
}

/** 返回成功解析帧数。 */
uint32_t SbusDecoder::totalFrames() const
{
  return __atomic_load_n(&_totalFrames, __ATOMIC_ACQUIRE);
}

/** 返回累计丢帧数。 */
uint32_t SbusDecoder::droppedFrames() const
{
  return invalidFrames() + receiverLostFrames();
}

/** 返回解析器检测到的格式错误帧数量。 */
uint32_t SbusDecoder::invalidFrames() const
{
  return __atomic_load_n(&_invalidFrames, __ATOMIC_ACQUIRE);
}

/** 返回接收机通过 SBUS 标志位报告的丢帧数量。 */
uint32_t SbusDecoder::receiverLostFrames() const
{
  return __atomic_load_n(&_receiverLostFrames, __ATOMIC_ACQUIRE);
}

/** 解包 22 字节负载中的 16 个连续 11 位通道。 */
bool SbusDecoder::decode(SbusFrame &frame)
{
  if (_buffer[0] != SbusStartByte || !validEndByte(_buffer[24]))
    {
      return false;
    }

  for (uint8_t channel = 0; channel < SbusFrame::ChannelCount; ++channel)
    {
      const uint16_t bitIndex = static_cast<uint16_t>(channel) * 11U;
      const uint8_t byteIndex = static_cast<uint8_t>(1U + bitIndex / 8U);
      const uint8_t bitOffset = static_cast<uint8_t>(bitIndex % 8U);

      const uint32_t packed =
          static_cast<uint32_t>(_buffer[byteIndex]) |
          (static_cast<uint32_t>(_buffer[byteIndex + 1U]) << 8U) |
          (static_cast<uint32_t>(_buffer[byteIndex + 2U]) << 16U);
      const uint16_t rawValue =
          static_cast<uint16_t>((packed >> bitOffset) & 0x07ffU);
      frame.values[channel] = sbusRawToPwm(rawValue);
    }

  frame.channelCount = SbusFrame::ChannelCount;
  frame.frameLost = (_buffer[SbusFlagsIndex] & SbusFrameLostMask) != 0;
  frame.failsafe = (_buffer[SbusFlagsIndex] & SbusFailsafeMask) != 0;
  return true;
}

/** 从错误帧内部恢复到可能存在的下一帧帧头。 */
void SbusDecoder::recoverAfterInvalidFrame()
{
  size_t nextStart = FrameSize;
  for (size_t index = 1; index < FrameSize; ++index)
    {
      if (_buffer[index] == SbusStartByte)
        {
          nextStart = index;
          break;
        }
    }

  if (nextStart == FrameSize)
    {
      _count = 0;
      return;
    }

  _count = FrameSize - nextStart;
  memmove(_buffer, &_buffer[nextStart], _count);
}
