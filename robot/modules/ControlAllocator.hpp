/**
 * @file ControlAllocator.hpp
 * @brief 声明订阅手动控制并发布电机、舵机输出的 Climbot 分配任务。
 */

#pragma once

#include "robot/common/ModuleBase.hpp"
#include "robot/configuration/ClimbotConfiguration.hpp"
#include "robot/control/ArmSafety.hpp"
#include "robot/control/ClimbotAllocation.hpp"
#include "robot/orb/Topics.hpp"

/** 第一版固定装配 ClimbotConfiguration 和 ClimbotAllocation。 */
class ControlAllocator final : public ModuleBase<ControlAllocator>
{
public:
  ControlAllocator();

  static int task_spawn(int argc, char *argv[]);
  static ControlAllocator *instantiate(int argc, char *argv[]);
  static int custom_command(int argc, char *argv[]);
  static int print_usage(const char *reason = nullptr);

  int print_status() override;
  void run() override;

private:
  static constexpr uint64_t ArmWarningIntervalMicroseconds = 5000000ULL;

  /** 使用 CA_ARM_SW 指定的开关推进上电安全互锁。 */
  bool updateArmState();

  /** 低频提示上电时没有处于关闭位置的安全开关。 */
  void warnIfArmSwitchUnsafe(uint64_t now);

  /** 返回当前安全开关位置；输入无效时返回 POSITION_NONE。 */
  uint8_t armSwitchPosition() const;

  /** 刷新分配系数和解锁开关映射。 */
  bool updateParameters();

  ClimbotConfiguration _configuration;
  ClimbotAllocation _allocation;
  uorb::Subscription<ManualControl> _manualSubscription;
  uorb::Subscription<ManualControlSwitches> _switchSubscription;
  uorb::Subscription<ParameterUpdate> _parameterSubscription;
  uorb::Publication<ActuatorMotors> _motorsPublication;
  uorb::Publication<ActuatorServos> _servosPublication;
  ManualControl _manual;
  ManualControlSwitches _switches;
  ArmSafety _armSafety;
  int32_t _armSwitch;
  uint32_t _allocationCount;
  uint64_t _lastArmWarning;
  bool _haveManual;
  bool _haveSwitches;
  bool _armed;
};
