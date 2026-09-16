/**
 * @file RcConfig.hpp
 * @brief 定义遥控通道校准参数、功能映射和纯计算辅助函数。
 */

#pragma once

#include "msg/InputRc.hpp"
#include "msg/RcChannels.hpp"

#include <stdint.h>

/**
 * @struct RcChannelCalibration
 * @brief 保存单个通道的 MIN、MAX、TRIM、DEADZONE 和 REV 参数。
 */
struct RcChannelCalibration
{
  uint16_t minimum{1000};
  uint16_t maximum{2000};
  uint16_t trim{1500};
  uint16_t deadzone{20};
  bool reversed{false};
};

/**
 * @class RcConfig
 * @brief 集中保存 16 个通道的校准值以及 MK32 默认功能映射。
 *
 * 第一版使用内存默认值；后续 Param/Flash 模块将更新这些字段，而 rc_update
 * 的归一化代码无需改变。
 */
class RcConfig
{
public:
  static constexpr uint8_t Unmapped = 0xff;

  /** 创建默认 1000/1500/2000 校准和 MK32 通道映射。 */
  RcConfig();

  /** 从 ParamManager 重新读取全部 RC 校准值和功能映射。 */
  bool refresh();

  /** 返回指定物理通道的校准参数。 */
  const RcChannelCalibration &calibration(uint8_t channel) const;

  /** 返回指定控制功能映射到的物理通道索引。 */
  uint8_t functionChannel(uint8_t function) const;

  /** 检查四个主摇杆映射及对应校准参数是否有效。 */
  bool valid() const;

private:
  RcChannelCalibration _calibration[InputRc::MAX_CHANNELS];
  uint8_t _functionChannel[RcChannels::FUNCTION_COUNT];
};

/** 使用分段线性函数将原始 PWM 值校准到 -1 到 1。 */
float normalizeRcChannel(uint16_t rawValue,
                         const RcChannelCalibration &calibration);

/** 把归一化通道转换为 OFF、MIDDLE、ON 三段开关状态。 */
uint8_t rcSwitchPosition(float normalizedValue);
