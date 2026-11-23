/** @file RobotControl.hpp
 * @brief 声明手动输入到机器人控制目标的统一生成模块。
 */
#pragma once

#include "robot/common/ModuleBase.hpp"
#include "robot/work_queue/ScheduledWorkItem.hpp"
#include "robot/work_queue/WorkQueueConfigurations.hpp"
#include "robot/orb/SubscriptionCallback.hpp"
#include "robot/orb/Topics.hpp"
#include "robot/params/ModuleParams.hpp"
#include "robot/params/Param.hpp"

/** 订阅手动输入和Failsafe标志，应用策略后发布唯一控制目标。 */
class RobotControl final : public ModuleBase<RobotControl>, public ModuleParams,
                               public robot_wq::ScheduledWorkItem
{
public:
  /** 绑定手动输入、Failsafe状态、参数和控制目标topic。 */
  RobotControl();
  ~RobotControl() override;
  /** PX4-style module-owned startup: parse, new, store, init. */
  static int task_spawn(int argc, char *argv[]);
  bool init();
  /** 输出生命周期命令帮助。 */
  static int print_usage(const char *reason = nullptr);
  /** 输出当前目标和已经应用的安全策略。 */
  int print_status() override;
  /** 生成并发布一次控制目标，测试可传入确定时间。 */
  void update(uint64_t now = 0);
  /** 消息回调及20ms兜底发布控制目标。 */
  void Run() override;

protected:
  bool stop_work_queue() override;

private:
  void unregisterCallbacks();
  void publishStoppedState();
  static constexpr uint64_t BackupIntervalUs = 20000;
  static constexpr uint64_t MinimumIntervalUs = 20000;
  /** 按0保持、1归零、2固定值处理单个控制量。 */
  static float applyAction(int32_t action, float lastValue, float fixedValue);
  /** 判断Failsafe状态心跳是否仍然有效。 */
  static bool fresh(uint64_t timestamp, uint64_t now);
  /** 检查目标处理参数范围。 */
  bool refreshPolicy();

  uorb::SubscriptionCallbackWorkItem<ManualControl> _manualSubscription;
  uorb::SubscriptionCallbackWorkItem<FailsafeStatus> _failsafeSubscription;
  uorb::SubscriptionCallbackWorkItem<ParameterUpdate> _parameterSubscription;
  uorb::Publication<RobotControlSetpoint> _setpointPublication;
  ParamInt _paramMotorAction;
  ParamFloat _paramMotorValue;
  ParamInt _paramSteeringAction;
  ParamFloat _paramSteeringValue;
  os::Mutex _stateMutex;
  ManualControl _manual{};
  ManualControl _lastValidManual{};
  FailsafeStatus _failsafe{};
  RobotControlSetpoint _setpoint{};
  bool _haveValidInput{false};
  bool _failsafeSeen{false};
};
