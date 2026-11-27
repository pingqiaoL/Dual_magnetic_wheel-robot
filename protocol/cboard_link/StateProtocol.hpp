#pragma once

#include <stddef.h>
#include <stdint.h>

namespace cboard_link
{
// Shared wire format, little-endian fields. Keep both endpoint copies identical.
enum class Kind : uint8_t { Query = 1, Reply = 2, Event = 3 };

struct Frame
{
    Kind kind;
    uint32_t session;
    uint32_t query;
    uint64_t revision;
    bool ready;
};

constexpr size_t FRAME_BYTES = 23;
constexpr uint8_t SYNC_FIRST = 0xA5;
constexpr uint8_t SYNC_SECOND = 0x5A;
constexpr uint8_t VERSION = 1;

inline uint16_t crc16(const uint8_t *bytes, size_t count)
{
    uint16_t crc = 0xFFFF;
    for (size_t index = 0; index < count; ++index)
    {
        crc ^= uint16_t(bytes[index]) << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
        {
            crc = uint16_t((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
        }
    }
    return crc;
}

inline void put_le(uint8_t *bytes, size_t count, uint64_t value)
{
    for (size_t index = 0; index < count; ++index)
    {
        bytes[index] = uint8_t(value >> (8 * index));
    }
}

inline uint64_t get_le(const uint8_t *bytes, size_t count)
{
    uint64_t value = 0;
    for (size_t index = 0; index < count; ++index)
    {
        value |= uint64_t(bytes[index]) << (8 * index);
    }
    return value;
}

inline void encode(const Frame &frame, uint8_t (&bytes)[FRAME_BYTES])
{
    bytes[0] = SYNC_FIRST;
    bytes[1] = SYNC_SECOND;
    bytes[2] = VERSION;
    bytes[3] = static_cast<uint8_t>(frame.kind);
    put_le(bytes + 4, 4, frame.session);
    put_le(bytes + 8, 4, frame.query);
    put_le(bytes + 12, 8, frame.revision);
    bytes[20] = frame.ready;
    put_le(bytes + 21, 2, crc16(bytes + 2, 19));
}

class Decoder
{
public:
    bool feed(uint8_t byte, Frame &frame)
    {
        _bytes[_size++] = byte;
        while (_size > 1 && (_bytes[0] != SYNC_FIRST || _bytes[1] != SYNC_SECOND))
        {
            discard_first();
        }
        if (_size < FRAME_BYTES)
        {
            return false;
        }

        const auto kind = static_cast<Kind>(_bytes[3]);
        const bool valid = _bytes[2] == VERSION && _bytes[20] <= 1 &&
                           (kind == Kind::Query || kind == Kind::Reply || kind == Kind::Event) &&
                           crc16(_bytes + 2, 19) == get_le(_bytes + 21, 2);
        if (!valid)
        {
            discard_first();
            return false;
        }
        frame = {kind, uint32_t(get_le(_bytes + 4, 4)), uint32_t(get_le(_bytes + 8, 4)),
                 get_le(_bytes + 12, 8), _bytes[20] != 0};
        _size = 0;
        return true;
    }

private:
    void discard_first()
    {
        --_size;
        for (size_t index = 0; index < _size; ++index)
        {
            _bytes[index] = _bytes[index + 1];
        }
    }

    uint8_t _bytes[FRAME_BYTES]{};
    size_t _size{0};
};
} // namespace cboard_link
