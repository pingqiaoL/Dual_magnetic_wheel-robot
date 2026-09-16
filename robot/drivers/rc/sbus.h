/**
 * @file sbus.h
 * @brief 声明与硬件无关的 SBUS 字节流解析器。
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @struct SbusFrame
 * @brief 保存一帧 SBUS 解码后的 16 路通道和链路标志。
 */
struct SbusFrame
{
  static constexpr uint8_t ChannelCount = 16;

  uint16_t values[ChannelCount]{};
  uint8_t channelCount{0};
  bool failsafe{false};
  bool frameLost{false};
};

/**
 * @class SbusDecoder
 * @brief 从任意分段到达的串口字节中寻找并解析 25 字节 SBUS 帧。
 */
class SbusDecoder
{
public:
  static constexpr size_t FrameSize = 25;

  /** 创建处于等待帧头状态的解析器。 */
  SbusDecoder();

  /** 清除未完成帧和统计信息。 */
  void reset();

  /** 输入一个字节；完整有效帧产生时写入 frame 并返回 true。 */
  bool feed(uint8_t byte, uint64_t timestampMicroseconds, SbusFrame &frame);

  /** 返回成功解码的总帧数。 */
  uint32_t totalFrames() const;

  /** 返回格式错误与接收机报告丢帧之和，保留给兼容调用。 */
  uint32_t droppedFrames() const;

  /** 返回帧头已经同步、但帧尾不符合 SBUS 格式的累计数量。 */
  uint32_t invalidFrames() const;

  /** 返回由 SBUS flags 中 frame_lost 位报告的累计丢帧数量。 */
  uint32_t receiverLostFrames() const;

private:
  /** 检查帧头、帧尾并解码 16 个 11 位通道。 */
  bool decode(SbusFrame &frame);

  /** 在坏帧中寻找下一个 0x0f 帧头，尽量恢复同步。 */
  void recoverAfterInvalidFrame();

  uint8_t _buffer[FrameSize];
  size_t _count;
  uint32_t _totalFrames;
  uint32_t _invalidFrames;
  uint32_t _receiverLostFrames;
};
