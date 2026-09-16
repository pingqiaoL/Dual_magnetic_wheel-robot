/**
 * @file damiao_protocol_test.cpp
 * @brief 在主机上验证达妙协议帧 ID、字节序、管理命令和反馈解析。
 */

#include "protocol/can/DamiaoProtocol.hpp"

#include <assert.h>
#include <math.h>
#include <stdio.h>

int main()
{
  CanFrame frame;
  assert(DamiaoProtocol::encodeEnable(3, DamiaoMode::PositionVelocity, frame));
  assert(frame.id == 0x103 && frame.length == 8 && frame.data[7] == 0xfc);
  for (unsigned index = 0; index < 7; ++index)
    {
      assert(frame.data[index] == 0xff);
    }

  assert(DamiaoProtocol::encodeVelocity(2, 1.0f, frame));
  assert(frame.id == 0x202 && frame.length == 4);
  assert(frame.data[0] == 0x00 && frame.data[1] == 0x00 &&
         frame.data[2] == 0x80 && frame.data[3] == 0x3f);

  assert(DamiaoProtocol::encodePositionVelocity(1, 1.0f, 2.0f, frame));
  assert(frame.id == 0x101 && frame.length == 8);
  assert(frame.data[2] == 0x80 && frame.data[3] == 0x3f);
  assert(frame.data[6] == 0x00 && frame.data[7] == 0x40);

  frame = CanFrame{};
  frame.id = 0;
  frame.length = 8;
  frame.data[0] = 0x23; // 状态 2，电机 ID 3
  frame.data[1] = 0x80;
  frame.data[2] = 0x00;
  frame.data[3] = 0x80;
  frame.data[4] = 0x08;
  frame.data[5] = 0x00;
  frame.data[6] = 42;
  frame.data[7] = 39;

  DamiaoLimits limits;
  limits.positionMax = 12.5f;
  limits.velocityMax = 30.0f;
  limits.torqueMax = 10.0f;
  DamiaoFeedback feedback;
  assert(DamiaoProtocol::decodeFeedback(frame, limits, feedback));
  assert(feedback.motorId == 3 && feedback.error == 2);
  assert(fabsf(feedback.position) < 0.001f);
  assert(fabsf(feedback.velocity) < 0.02f);
  assert(fabsf(feedback.torque) < 0.01f);
  assert(feedback.mosTemperature == 42.0f);
  assert(feedback.coilTemperature == 39.0f);

  assert(!DamiaoProtocol::encodeVelocity(0x700, 1.0f, frame));
  printf("damiao protocol test passed\n");
  return 0;
}
