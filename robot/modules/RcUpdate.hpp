/**
 * @file RcUpdate.hpp
 * @brief 声明 input_rc 校准、映射、归一化和上层手动控制消息发布模块。
 */

#pragma once

#include "robot/common/ModuleBase.hpp"
#include "robot/modules/RcConfig.hpp"
#include "robot/orb/Topics.hpp"

#include <stdint.h>

/**
 * @class RcUpdate
 * @brief 订阅 input_rc，输出 rc_channels、manual_control_input 和开关状态。
 */
class RcUpdate final : public ModuleBase<RcUpdate>
{
public:
  /** 绑定遥控链路所需的订阅器和发布器。 */
  RcUpdate();

  /** 创建 rc_update 独立线程。 */
  static int task_spawn(int argc, char *argv[]);

  /** 创建 RcUpdate 对象。 */
  static RcUpdate *instantiate(int argc, char *argv[]);

  /** 处理模块自定义命令。 */
  static int custom_command(int argc, char *argv[]);

  /** 打印 rc_update 命令帮助。 */
  static int print_usage(const char *reason = nullptr);

  /** 打印处理统计、归一化摇杆、开关位置和信号状态。 */
  int print_status() override;

  /** 周期检查 input_rc 更新并执行校准与映射。 */
  void run() override;

private:
  /** 校准一帧输入并发布全部上层遥控 topic。 */
  void processInput(const InputRc &input, uint64_t now);

  /** 超时没有 input_rc 更新时发布一次失联状态。 */
  void publishTimeoutIfNeeded(uint64_t now);

  /** 从归一化通道中读取指定功能，映射无效时返回默认值。 */
  float functionValue(const RcChannels &channels, uint8_t function,
                      float defaultValue) const;

  static constexpr uint64_t InputTimeoutMicroseconds = 500000;

  RcConfig _config;
  uorb::Subscription<InputRc> _inputSubscription;
  uorb::Subscription<ParameterUpdate> _parameterSubscription;
  uorb::Publication<RcChannels> _channelsPublication;
  uorb::Publication<ManualControl> _manualPublication;
  uorb::Publication<ManualControlSwitches> _switchesPublication;
  uint64_t _lastInputTimestamp;
  uint8_t _previousChannelCount;
  uint8_t _stableFrameCount;
  uint32_t _processedFrames;
  bool _signalLost;
  bool _timeoutPublished;
};
