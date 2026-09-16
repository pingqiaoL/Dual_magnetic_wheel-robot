/**
 * @file FlashParamStorage.cpp
 * @brief 使用 NuttX progmem 实现 STM32F407 参数快照的 A/B 原子保存。
 */

#include "robot/params/FlashParamStorage.hpp"

#include <nuttx/progmem.h>

#include <stddef.h>
#include <string.h>

namespace
{
constexpr uint32_t StorageMagic = 0x5041524dU;       // "PARM"
constexpr uint16_t StorageVersion = 1;
constexpr uint32_t StorageCommitted = 0x434f4d54U;   // "COMT"
constexpr uint32_t StorageErased = 0xffffffffU;

/** 每个 Flash 页开头的固定格式元数据。 */
struct StorageHeader
{
  uint32_t magic;
  uint16_t version;
  uint16_t recordCount;
  uint32_t sequence;
  uint32_t payloadBytes;
  uint32_t payloadCrc;
  uint32_t committed;
};

static_assert(sizeof(StorageHeader) == 24,
              "StorageHeader format must stay stable");

/** 计算与以太网/ZIP 相同多项式的 CRC32。 */
uint32_t crc32(const uint8_t *data, size_t length)
{
  uint32_t crc = 0xffffffffU;
  for (size_t index = 0; index < length; ++index)
    {
      crc ^= data[index];
      for (unsigned bit = 0; bit < 8; ++bit)
        {
          const uint32_t mask =
              static_cast<uint32_t>(-static_cast<int32_t>(crc & 1U));
          crc = (crc >> 1U) ^ (0xedb88320U & mask);
        }
    }
  return ~crc;
}

/** 使用有符号差值比较可回绕的 32 位序号。 */
bool sequenceNewer(uint32_t left, uint32_t right)
{
  return static_cast<int32_t>(left - right) > 0;
}
} // namespace

/** 保存参数页地址；实际扇区合法性会在擦除前通过 progmem 检查。 */
FlashParamStorage::FlashParamStorage(uintptr_t slotA, uintptr_t slotB,
                                     size_t slotSize)
    : _slotA(slotA),
      _slotB(slotB),
      _slotSize(slotSize),
      _activeSlot(0)
{
}

/** 选择两个页中序号最新且 CRC 有效的一页。 */
bool FlashParamStorage::load(ParamStorageSnapshot &snapshot)
{
  uint32_t sequenceA = 0;
  uint32_t sequenceB = 0;
  const bool validA = validate(_slotA, nullptr, &sequenceA);
  const bool validB = validate(_slotB, nullptr, &sequenceB);

  if (!validA && !validB)
    {
      _activeSlot = 0;
      return false;
    }

  _activeSlot = validA && (!validB || sequenceNewer(sequenceA, sequenceB))
                    ? _slotA
                    : _slotB;
  return validate(_activeSlot, &snapshot, nullptr);
}

/** 写入非活动页，并把提交标记作为最后一次 Flash 编程操作。 */
bool FlashParamStorage::save(const ParamStorageSnapshot &snapshot)
{
  if (snapshot.count > ParamMaxCount)
    {
      return false;
    }

  const size_t payloadBytes =
      static_cast<size_t>(snapshot.count) * sizeof(ParamStoredRecord);
  if (sizeof(StorageHeader) + payloadBytes > _slotSize)
    {
      return false;
    }

  const uintptr_t target = _activeSlot == _slotA ? _slotB : _slotA;
  const ssize_t page = up_progmem_getpage(target);
  if (page < 0 || up_progmem_getaddress(static_cast<size_t>(page)) != target ||
      up_progmem_erasesize(static_cast<size_t>(page)) != _slotSize)
    {
      return false;
    }

  if (up_progmem_eraseblock(static_cast<size_t>(page)) !=
      static_cast<ssize_t>(_slotSize))
    {
      return false;
    }

  StorageHeader header{};
  header.magic = StorageMagic;
  header.version = StorageVersion;
  header.recordCount = snapshot.count;
  header.sequence = snapshot.sequence;
  header.payloadBytes = static_cast<uint32_t>(payloadBytes);
  header.payloadCrc = crc32(
      reinterpret_cast<const uint8_t *>(snapshot.records), payloadBytes);
  header.committed = StorageErased;

  if (up_progmem_write(target, &header, sizeof(header)) !=
      static_cast<ssize_t>(sizeof(header)))
    {
      return false;
    }

  if (payloadBytes > 0 &&
      up_progmem_write(target + sizeof(header), snapshot.records,
                       payloadBytes) != static_cast<ssize_t>(payloadBytes))
    {
      return false;
    }

  const uint32_t committed = StorageCommitted;
  if (up_progmem_write(target + offsetof(StorageHeader, committed),
                       &committed, sizeof(committed)) !=
      static_cast<ssize_t>(sizeof(committed)))
    {
      return false;
    }

  uint32_t verifiedSequence = 0;
  if (!validate(target, nullptr, &verifiedSequence) ||
      verifiedSequence != snapshot.sequence)
    {
      return false;
    }

  _activeSlot = target;
  return true;
}

/** 验证一个参数页，并按需复制有效记录。 */
bool FlashParamStorage::validate(uintptr_t address,
                                 ParamStorageSnapshot *snapshot,
                                 uint32_t *sequence) const
{
  StorageHeader header{};
  memcpy(&header, reinterpret_cast<const void *>(address), sizeof(header));

  if (header.magic != StorageMagic || header.version != StorageVersion ||
      header.committed != StorageCommitted ||
      header.recordCount > ParamMaxCount ||
      header.payloadBytes !=
          static_cast<uint32_t>(header.recordCount *
                                sizeof(ParamStoredRecord)) ||
      sizeof(StorageHeader) + header.payloadBytes > _slotSize)
    {
      return false;
    }

  const uint8_t *payload = reinterpret_cast<const uint8_t *>(
      address + sizeof(StorageHeader));
  if (crc32(payload, header.payloadBytes) != header.payloadCrc)
    {
      return false;
    }

  if (sequence != nullptr)
    {
      *sequence = header.sequence;
    }

  if (snapshot != nullptr)
    {
      memset(snapshot, 0, sizeof(*snapshot));
      snapshot->sequence = header.sequence;
      snapshot->count = header.recordCount;
      if (header.payloadBytes > 0)
        {
          memcpy(snapshot->records, payload, header.payloadBytes);
        }
    }

  return true;
}
