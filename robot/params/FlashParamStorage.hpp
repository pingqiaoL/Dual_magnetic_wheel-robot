/**
 * @file FlashParamStorage.hpp
 * @brief 声明基于两个独立 Flash 擦除扇区的掉电安全参数后端。
 */

#pragma once

#include "robot/params/ParamStorage.hpp"

#include <stddef.h>
#include <stdint.h>

/** 使用 A/B 扇区、CRC 和最后提交标记保存参数。 */
class FlashParamStorage final : public ParamStorage
{
public:
  /** 绑定两个已经从链接脚本中排除的 Flash 扇区。 */
  FlashParamStorage(uintptr_t slotA, uintptr_t slotB, size_t slotSize);

  /** 读取序号较新的有效参数页。 */
  bool load(ParamStorageSnapshot &snapshot) override;

  /** 擦除非活动页、写入新快照并最后写入提交标记。 */
  bool save(const ParamStorageSnapshot &snapshot) override;

private:
  /** 校验页头、边界、提交标记和 CRC。 */
  bool validate(uintptr_t address, ParamStorageSnapshot *snapshot,
                uint32_t *sequence) const;

  uintptr_t _slotA;
  uintptr_t _slotB;
  size_t _slotSize;
  uintptr_t _activeSlot;
};
