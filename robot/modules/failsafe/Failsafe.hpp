/** @file Failsafe.hpp
 * @brief 检测手动控制丢失，只发布故障标志。
 */
#pragma once
#include "robot/common/ModuleBase.hpp"
#include "robot/work_queue/ScheduledWorkItem.hpp"
#include "robot/work_queue/WorkQueueConfigurations.hpp"
#include "robot/orb/SubscriptionCallback.hpp"
#include "robot/orb/Topics.hpp"
#include "robot/params/ModuleParams.hpp"
#include "robot/params/Param.hpp"

/** 将RC有效性检测与目标生成解耦的独立故障检测模块。 */
class Failsafe final : public ModuleBase<Failsafe>, public ModuleParams,
                               public robot_wq::ScheduledWorkItem
{
public:
  /** 绑定手动输入和Failsafe状态topic。 */
  Failsafe();
  ~Failsafe() override;
  /** PX4-style module-owned startup: parse, new, store, init. */
  static int task_spawn(int argc, char *argv[]);
  bool init();
  /** 输出命令帮助。 */
  static int print_usage(const char *reason = nullptr);
  /** 输出当前RC丢失状态。 */
  int print_status() override;
  /** 处理一次输入并发布故障标志，测试可传入确定时间。 */
  void update(uint64_t now = 0);
  /** 消息回调及20ms兜底发布故障状态心跳。 */
  void Run() override;

protected:
  bool stop_work_queue() override;

private:
  void unregisterCallbacks();
  void publishStoppedState();
  static constexpr uint64_t BackupIntervalUs = 20000;
  static constexpr uint64_t MinimumIntervalUs = 20000;
  /** 判断手动输入是否有效且未超过配置的超时时间。 */
  static bool fresh(const ManualControl &manual, uint64_t now);
  /** 检查超时参数范围并更新RC超时时间。 */
  bool refreshPolicy();
  uorb::SubscriptionCallbackWorkItem<ManualControl> _manualSubscription;
  uorb::SubscriptionCallbackWorkItem<ParameterUpdate> _parameterSubscription;
  uorb::Publication<FailsafeStatus> _statusPublication;
  ParamFloat _paramRcTimeout;
  os::Mutex _stateMutex;
  ManualControl _manual{};
  FailsafeStatus _status{};
  uint64_t _timeoutUs{500000ULL};
};
