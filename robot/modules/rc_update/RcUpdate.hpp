/**
 * @file RcUpdate.hpp
 * @brief 声明 input_rc 校准、映射、归一化和上层手动控制消息发布模块。
 */

#pragma once

#include "robot/common/ModuleBase.hpp"
#include "robot/work_queue/ScheduledWorkItem.hpp"
#include "robot/work_queue/WorkQueueConfigurations.hpp"
#include "robot/orb/SubscriptionCallback.hpp"
#include "robot/modules/rc_update/RcConfig.hpp"
#include "robot/orb/Topics.hpp"
#include "robot/params/ModuleParams.hpp"

#include <stdint.h>

/**
 * @class RcUpdate
 * @brief 订阅 input_rc，输出 rc_channels、manual_control_input 和开关状态。
 */
class RcUpdate final : public ModuleBase<RcUpdate>, public ModuleParams,
                               public robot_wq::ScheduledWorkItem
{
public:
  /** 绑定遥控链路所需的订阅器和发布器。 */
  RcUpdate();
  ~RcUpdate() override;


  /** PX4-style module-owned startup: parse, new, store, init. */
  static int task_spawn(int argc, char *argv[]);
  bool init();

  /** 打印 rc_update 命令帮助。 */
  static int print_usage(const char *reason = nullptr);

  /** 打印处理统计、归一化摇杆、开关位置和信号状态。 */
  int print_status() override;

  /** 消息回调或10ms兜底触发一次校准与映射。 */
  void Run() override;

protected:
  bool stop_work_queue() override;

private:
  void unregisterCallbacks();
  void publishStoppedState();
  static constexpr uint64_t BackupIntervalUs = 10000;
  static constexpr uint64_t MinimumIntervalUs = 10000;
  /** 校准一帧输入并发布全部上层遥控 topic。 */
  void processInput(const InputRc &input, uint64_t now);

  /** 超时没有 input_rc 更新时发布一次失联状态。 */
  void publishTimeoutIfNeeded(uint64_t now);

  /** 从归一化通道中读取指定功能，映射无效时返回默认值。 */
  float functionValue(const RcChannels &channels, uint8_t function,
                      float defaultValue) const;

  static constexpr uint64_t InputTimeoutMicroseconds = 500000;

  os::Mutex _stateMutex;
  RcConfig _config;
  uorb::SubscriptionCallbackWorkItem<InputRc> _inputSubscription;
  uorb::SubscriptionCallbackWorkItem<ParameterUpdate> _parameterSubscription;
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
