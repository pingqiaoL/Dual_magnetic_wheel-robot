/**
 * @file RcConfig.cpp
 * @brief 实现 MK32 默认映射、校准检查和通道归一化算法。
 */

#include "robot/modules/RcConfig.hpp"

#include "msg/ManualControlSwitches.hpp"
#include "robot/params/ParamManager.hpp"

#include <math.h>
#include <stdio.h>

namespace
{
/** 将浮点数限制在给定范围内。 */
float constrain(float value, float minimum, float maximum)
{
  if (value < minimum)
    {
      return minimum;
    }

  if (value > maximum)
    {
      return maximum;
    }

  return value;
}
} // namespace

/** 设置 MK32 默认映射：通道1到4为四个摇杆，通道5到16为开关。 */
RcConfig::RcConfig()
{
  for (uint8_t function = 0; function < RcChannels::FUNCTION_COUNT;
       ++function)
    {
      _functionChannel[function] = Unmapped;
    }

  _functionChannel[RcChannels::FUNCTION_ROLL] = 0;
  _functionChannel[RcChannels::FUNCTION_PITCH] = 1;
  _functionChannel[RcChannels::FUNCTION_THROTTLE] = 2;
  _functionChannel[RcChannels::FUNCTION_YAW] = 3;

  for (uint8_t switchIndex = 0;
       switchIndex < RcChannels::SWITCH_COUNT; ++switchIndex)
    {
      _functionChannel[RcChannels::FUNCTION_SWITCH_FIRST + switchIndex] =
          static_cast<uint8_t>(4U + switchIndex);
    }

  (void)refresh();
}

/** 从参数系统读取通道校准和从1开始的功能映射。 */
bool RcConfig::refresh()
{
  ParamManager &manager = ParamManager::instance();
  bool complete = true;
  char name[ParamNameLength];

  for (uint8_t channel = 0; channel < InputRc::MAX_CHANNELS; ++channel)
    {
      const unsigned number = static_cast<unsigned>(channel) + 1U;
      float value = 0.0f;

      snprintf(name, sizeof(name), "RC%u_MIN", number);
      complete = manager.get(name, value) && complete;
      _calibration[channel].minimum = static_cast<uint16_t>(value);

      snprintf(name, sizeof(name), "RC%u_MAX", number);
      complete = manager.get(name, value) && complete;
      _calibration[channel].maximum = static_cast<uint16_t>(value);

      snprintf(name, sizeof(name), "RC%u_TRIM", number);
      complete = manager.get(name, value) && complete;
      _calibration[channel].trim = static_cast<uint16_t>(value);

      snprintf(name, sizeof(name), "RC%u_DZ", number);
      complete = manager.get(name, value) && complete;
      _calibration[channel].deadzone = static_cast<uint16_t>(value);

      snprintf(name, sizeof(name), "RC%u_REV", number);
      complete = manager.get(name, value) && complete;
      _calibration[channel].reversed = value < 0.0f;
    }

  struct Mapping
  {
    const char *name;
    uint8_t function;
  };

  static const Mapping mainMappings[] = {
      {"RC_MAP_THROTTLE", RcChannels::FUNCTION_THROTTLE},
      {"RC_MAP_ROLL", RcChannels::FUNCTION_ROLL},
      {"RC_MAP_PITCH", RcChannels::FUNCTION_PITCH},
      {"RC_MAP_YAW", RcChannels::FUNCTION_YAW},
  };

  for (const Mapping &mapping : mainMappings)
    {
      int32_t channel = 0;
      complete = manager.get(mapping.name, channel) && complete;
      _functionChannel[mapping.function] =
          channel > 0 && channel <= InputRc::MAX_CHANNELS
              ? static_cast<uint8_t>(channel - 1)
              : Unmapped;
    }

  for (uint8_t switchIndex = 0;
       switchIndex < RcChannels::SWITCH_COUNT; ++switchIndex)
    {
      snprintf(name, sizeof(name), "RC_MAP_SW%u",
               static_cast<unsigned>(switchIndex) + 1U);
      int32_t channel = 0;
      complete = manager.get(name, channel) && complete;
      _functionChannel[RcChannels::FUNCTION_SWITCH_FIRST + switchIndex] =
          channel > 0 && channel <= InputRc::MAX_CHANNELS
              ? static_cast<uint8_t>(channel - 1)
              : Unmapped;
    }

  return complete;
}

/** 返回指定通道的校准参数；调用者必须传入 0 到 15。 */
const RcChannelCalibration &RcConfig::calibration(uint8_t channel) const
{
  return _calibration[channel];
}

/** 返回功能映射；未映射时返回 0xff。 */
uint8_t RcConfig::functionChannel(uint8_t function) const
{
  if (function >= RcChannels::FUNCTION_COUNT)
    {
      return Unmapped;
    }

  return _functionChannel[function];
}

/** 检查主摇杆映射不越界，且每个通道满足 MIN < TRIM < MAX。 */
bool RcConfig::valid() const
{
  for (uint8_t function = RcChannels::FUNCTION_THROTTLE;
       function <= RcChannels::FUNCTION_YAW; ++function)
    {
      const uint8_t channel = functionChannel(function);
      if (channel >= InputRc::MAX_CHANNELS)
        {
          return false;
        }

      const RcChannelCalibration &item = calibration(channel);
      if (!(item.minimum < item.trim && item.trim < item.maximum) ||
          item.deadzone >= item.trim - item.minimum ||
          item.deadzone >= item.maximum - item.trim)
        {
          return false;
        }
    }

  return true;
}

/** 应用死区、正负两侧独立比例和反向设置。 */
float normalizeRcChannel(uint16_t rawValue,
                         const RcChannelCalibration &calibration)
{
  if (!(calibration.minimum < calibration.trim &&
        calibration.trim < calibration.maximum))
    {
      return 0.0f;
    }

  const float value = static_cast<float>(rawValue);
  const float minimum = static_cast<float>(calibration.minimum);
  const float maximum = static_cast<float>(calibration.maximum);
  const float trim = static_cast<float>(calibration.trim);
  const float lowerDeadzone = trim - calibration.deadzone;
  const float upperDeadzone = trim + calibration.deadzone;

  float normalized = 0.0f;
  if (value < lowerDeadzone && lowerDeadzone > minimum)
    {
      normalized = -(lowerDeadzone - value) / (lowerDeadzone - minimum);
    }
  else if (value > upperDeadzone && maximum > upperDeadzone)
    {
      normalized = (value - upperDeadzone) / (maximum - upperDeadzone);
    }

  normalized = constrain(normalized, -1.0f, 1.0f);
  if (calibration.reversed)
    {
      normalized = -normalized;
    }

  return isfinite(normalized) ? normalized : 0.0f;
}

/** 使用正负 0.5 阈值识别两段和三段开关位置。 */
uint8_t rcSwitchPosition(float normalizedValue)
{
  if (normalizedValue > 0.5f)
    {
      return ManualControlSwitches::POSITION_ON;
    }

  if (normalizedValue < -0.5f)
    {
      return ManualControlSwitches::POSITION_OFF;
    }

  return ManualControlSwitches::POSITION_MIDDLE;
}
