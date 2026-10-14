/** @file Failsafe.hpp
 * @brief 检测手动控制丢失，只发布故障标志。
 */
#pragma once
#include "robot/common/WorkQueueModuleBase.hpp"
#include "robot/orb/SubscriptionCallback.hpp"
#include "robot/orb/Topics.hpp"
#include "robot/params/ModuleParams.hpp"
#include "robot/params/Param.hpp"

/** 将RC有效性检测与目标生成解耦的独立故障检测模块。 */
class Failsafe final : public WorkQueueModuleBase<Failsafe>, public ModuleParams
{
public:
  /** 绑定手动输入和Failsafe状态topic。 */
  Failsafe();
  /** 创建模块对象，不接受启动参数。 */
  static Failsafe *instantiate(int argc, char *argv[]);
  /** 输出命令帮助。 */
  static int print_usage(const char *reason = nullptr);
  /** 输出当前RC丢失状态。 */
  int print_status() override;
  /** 处理一次输入并发布故障标志，测试可传入确定时间。 */
  void update(uint64_t now = 0);
  /** 消息回调及20ms兜底发布故障状态心跳。 */
  void Run() override;

protected:
  bool initWorkQueue() override;
  void unregisterCallbacks() override;
  void onWorkQueueStopped() override;

private:
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
