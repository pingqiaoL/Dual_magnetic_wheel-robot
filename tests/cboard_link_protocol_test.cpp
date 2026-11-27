#include "protocol/cboard_link/StateProtocol.hpp"

#include <assert.h>
#include <stdio.h>

using namespace cboard_link;

int main()
{
  const uint8_t crcExample[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  assert(crc16(crcExample, sizeof(crcExample)) == 0x29B1);

  uint8_t frameBytes[FRAME_BYTES]{};
  encode({Kind::Reply, 0xA55A1234, 0x0A0D0001, UINT64_C(0xDEADBEEF00010000), true}, frameBytes);
  Decoder decoder;
  Frame frame{};
  assert(!decoder.feed(0xFF, frame));
  assert(!decoder.feed(SYNC_FIRST, frame));
  // Corrupt and truncate a frame before sending the valid one.
  for (size_t index = 0; index < FRAME_BYTES - 2; ++index)
    {
      assert(!decoder.feed(frameBytes[index], frame));
    }
  for (size_t index = 0; index < FRAME_BYTES - 1; ++index)
    {
      assert(!decoder.feed(frameBytes[index], frame));
    }
  assert(decoder.feed(frameBytes[FRAME_BYTES - 1], frame));
  assert(frame.kind == Kind::Reply && frame.session == 0xA55A1234 && frame.query == 0x0A0D0001);
  assert(frame.revision == UINT64_C(0xDEADBEEF00010000) && frame.ready);
  puts("PASS: CBoard serial framing, CRC and recovery");
}
