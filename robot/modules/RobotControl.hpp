/** @file RobotControl.hpp
 * @brief 声明手动输入到机器人控制目标的统一生成模块。
 */
#pragma once

#include "robot/common/ModuleBase.hpp"
#include "robot/orb/Topics.hpp"
#include "robot/params/ModuleParams.hpp"
#include "robot/params/Param.hpp"

/** 订阅手动输入和Failsafe标志，应用策略后发布唯一控制目标。 */
class RobotControl final : public ModuleBase<RobotControl>, public ModuleParams
{
public:
  /** 绑定手动输入、Failsafe状态、参数和控制目标topic。 */
  RobotControl();
  /** 创建105优先级的50Hz目标生成任务。 */
  static int task_spawn(int argc, char *argv[]);
  /** 创建不接受启动参数的目标生成对象。 */
  static RobotControl *instantiate(int argc, char *argv[]);
  /** 返回统一命令路由使用的名称。 */
  static const char *command_name() { return "robot_control"; }
  /** 输出生命周期命令帮助。 */
  static int print_usage(const char *reason = nullptr);
  /** 输出当前目标和已经应用的安全策略。 */
  int print_status() override;
  /** 生成并发布一次控制目标，测试可传入确定时间。 */
  void update(uint64_t now = 0);
  /** 以50Hz持续发布控制目标。 */
  void run() override;

private:
  /** 按0保持、1归零、2固定值处理单个控制量。 */
  static float applyAction(int32_t action, float lastValue, float fixedValue);
  /** 判断Failsafe状态心跳是否仍然有效。 */
  static bool fresh(uint64_t timestamp, uint64_t now);
  /** 检查目标处理参数范围。 */
  bool refreshPolicy();

  uorb::Subscription<ManualControl> _manualSubscription;
  uorb::Subscription<FailsafeStatus> _failsafeSubscription;
  uorb::Subscription<ParameterUpdate> _parameterSubscription;
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
