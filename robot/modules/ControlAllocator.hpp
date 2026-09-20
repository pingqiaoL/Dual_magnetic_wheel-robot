/** @file ControlAllocator.hpp
 * @brief 按CA_AIRFRAME创建构型模型，使用通用算法发布执行器输出。
 */
#pragma once
#include "robot/common/ModuleBase.hpp"
#include "robot/control/Allocation.hpp"
#include "robot/orb/Topics.hpp"

/** 只负责参数化控制分配，不判断SA开关或发送CAN使能命令。 */
class ControlAllocator final : public ModuleBase<ControlAllocator>
{
public:
  /** 接管工厂创建的模型对象，并绑定手动输入和输出topic。 */
  ControlAllocator(int32_t modelId, ActuatorEffectiveness *effectiveness);
  /** 释放拥有的构型对象。 */
  ~ControlAllocator() override;
  /** 创建105优先级的控制分配任务。 */
  static int task_spawn(int argc, char *argv[]);
  /** 从CA_AIRFRAME创建模型，兼容旧的-c climbot选项。 */
  static ControlAllocator *instantiate(int argc, char *argv[]);
  /** 提供统一路由使用的固定名称。 */
  static const char *command_name() { return "control_allocator"; }
  /** 显示帮助。 */
  static int print_usage(const char *reason = nullptr);
  /** 显示当前构型和分配状态。 */
  int print_status() override;
  /** 50Hz发布输出；输入过期或模型改变时发布无效输出。 */
  void run() override;

private:
  /** 模型选择的唯一创建位置，新增机型时增加一个分支。 */
  static ActuatorEffectiveness *createEffectiveness(int32_t modelId);
  /** 刷新矩阵；运行中变更模型需停止后重新启动模块。 */
  void updateParameters();
  const int32_t _modelId;
  ActuatorEffectiveness *_effectiveness;
  Allocation _allocation;
  uorb::Subscription<ManualControl> _manualSubscription;
  uorb::Subscription<ParameterUpdate> _parameterSubscription;
  uorb::Publication<ActuatorMotors> _motorsPublication;
  uorb::Publication<ActuatorServos> _servosPublication;
  /** 保护运行线程与NSH状态读取之间的矩阵状态及计数。 */
  os::Mutex _stateMutex;
  ManualControl _manual{};
  uint32_t _allocationCount{0};
  bool _modelValid{true};
};
