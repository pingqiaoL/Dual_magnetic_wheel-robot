/** @file Command.hpp
 * @brief 接收遥控状态、执行一次性上电互锁并发布解锁信息。
 */
#pragma once
#include "robot/common/WorkQueueModuleBase.hpp"
#include "robot/orb/SubscriptionCallback.hpp"
#include "robot/control/ArmSafety.hpp"
#include "robot/orb/Topics.hpp"
#include "robot/params/ModuleParams.hpp"
#include "robot/params/Param.hpp"

/** 安全工作队列中的管理模块，并自行处理手动输出禁止扩展命令。 */
class Command final : public WorkQueueModuleBase<Command>, public ModuleParams
{
public:
  /** 绑定遥控、开关和参数订阅，初始状态禁止解锁。 */
  Command();
  /** 创建安全管理对象，不接受额外启动参数。 */
  static Command *instantiate(int argc, char *argv[]);
  /** 输出命令帮助。 */
  static int print_usage(const char *reason = nullptr);
  /** 处理enable和disable扩展命令。 */
  static int custom_command(int argc, char *argv[]);
  /** 输出最新安全状态。 */
  int print_status() override;
  /** 单次读取并发布；默认读取后采时，主机验收可指定时间。 */
  void update(uint64_t now = 0);
  /** 单次更新安全状态；停止路径发布失锁消息。 */
  void Run() override;
  /** 供统一命令处理器原子设置输出禁止标志，不绕过遥控互锁。 */
  static void setOutputInhibited(bool inhibited);
  /** 查询NSH手动禁止状态。 */
  static bool outputInhibited();
  /** Flush deferred warnings from the independent runtime task. */
  static void reportWarnings();

protected:
  bool initWorkQueue() override;
  void unregisterCallbacks() override;
  void onWorkQueueStopped() override;

private:
  /** 刷新安全开关编号；映射变化时重新要求OFF。 */
  void updateParameters();
  /** 验证输入有效性和时间，返回安全开关位置。 */
  uint8_t switchPosition(uint64_t now) const;
  /** 推进一次性互锁并发布执行器安全状态。 */
  void updateSafety(uint64_t now);
  /** 判断消息是否在500毫秒有效期内。 */
  static bool fresh(uint64_t timestamp, uint64_t now);

  static bool _outputInhibited;
  static int32_t _warningSwitch;
  uorb::SubscriptionCallbackWorkItem<ManualControl> _manualSubscription;
  uorb::SubscriptionCallbackWorkItem<ManualControlSwitches> _switchSubscription;
  uorb::SubscriptionCallbackWorkItem<ParameterUpdate> _parameterSubscription;
  uorb::Publication<ActuatorArmed> _armedPublication;
  ParamInt _paramArmSwitch;
  /** 保护运行线程和NSH状态查询之间的非原子缓存。 */
  os::Mutex _stateMutex;
  ManualControl _manual{};
  ManualControlSwitches _switches{};
  ActuatorArmed _status{};
  ArmSafety _armSafety;
  int32_t _armSwitch{1};
  uint8_t _position{ManualControlSwitches::POSITION_NONE};
  uint64_t _lastWarning{0};
};
