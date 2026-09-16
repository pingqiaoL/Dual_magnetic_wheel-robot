/**
 * @file sbus_decoder_test.cpp
 * @brief 在主机上验证 SBUS 位解包、failsafe 标志和 RC 校准算法。
 */

#include "robot/drivers/rc/sbus.h"
#include "robot/modules/RcConfig.hpp"
#include "msg/ManualControlSwitches.hpp"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/** 把一个 11 位测试值写入标准 SBUS 通道负载。 */
void encodeChannel(uint8_t frame[SbusDecoder::FrameSize], uint8_t channel,
                   uint16_t value)
{
  const uint16_t firstBit = static_cast<uint16_t>(channel) * 11U;
  for (uint8_t bit = 0; bit < 11; ++bit)
    {
      if ((value & (1U << bit)) != 0)
        {
          const uint16_t payloadBit = firstBit + bit;
          frame[1U + payloadBit / 8U] |=
              static_cast<uint8_t>(1U << (payloadBit % 8U));
        }
    }
}

/** 生成一帧已知通道值并逐字节送入解析器。 */
SbusFrame decodeTestFrame(SbusDecoder &decoder, uint8_t flags)
{
  uint8_t bytes[SbusDecoder::FrameSize]{};
  bytes[0] = 0x0f;
  bytes[23] = flags;
  bytes[24] = 0x00;

  encodeChannel(bytes, 0, 200);
  encodeChannel(bytes, 1, 1800);

  for (uint8_t channel = 2; channel < SbusFrame::ChannelCount; ++channel)
    {
      encodeChannel(bytes, channel, 1024);
    }

  SbusFrame decoded;
  bool complete = false;
  for (size_t index = 0; index < sizeof(bytes); ++index)
    {
      complete = decoder.feed(bytes[index], 10000U + index * 100U, decoded) ||
                 complete;
    }

  assert(complete);
  return decoded;
}

/** 执行全部协议和校准断言。 */
int main()
{
  SbusDecoder decoder;
  SbusFrame frame = decodeTestFrame(decoder, 0);
  assert(frame.channelCount == 16);
  assert(frame.values[0] == 1000);
  assert(frame.values[1] == 2000);
  assert(frame.values[2] == 1515);
  assert(!frame.frameLost);
  assert(!frame.failsafe);

  frame = decodeTestFrame(decoder, 1U << 2);
  assert(frame.frameLost);
  assert(!frame.failsafe);

  frame = decodeTestFrame(decoder, 1U << 3);
  assert(frame.failsafe);
  assert(decoder.totalFrames() == 3);
  assert(decoder.droppedFrames() == 1);
  assert(decoder.invalidFrames() == 0);
  assert(decoder.receiverLostFrames() == 1);

  RcChannelCalibration calibration;
  assert(fabsf(normalizeRcChannel(1000, calibration) + 1.0f) < 0.001f);
  assert(fabsf(normalizeRcChannel(1500, calibration)) < 0.001f);
  assert(fabsf(normalizeRcChannel(2000, calibration) - 1.0f) < 0.001f);
  assert(rcSwitchPosition(-1.0f) == ManualControlSwitches::POSITION_OFF);
  assert(rcSwitchPosition(0.0f) == ManualControlSwitches::POSITION_MIDDLE);
  assert(rcSwitchPosition(1.0f) == ManualControlSwitches::POSITION_ON);

  calibration.reversed = true;
  assert(fabsf(normalizeRcChannel(1000, calibration) - 1.0f) < 0.001f);

  printf("[PASS] SBUS decoder and RC calibration tests passed.\n");
  return 0;
}
