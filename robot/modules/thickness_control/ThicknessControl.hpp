#pragma once

#include "robot/common/ModuleBase.hpp"
#include "robot/drivers/probe_lift/ProbeLift.hpp"
#include "robot/orb/Topics.hpp"
#include "robot/work_queue/ScheduledWorkItem.hpp"

/** 在解锁后复位测厚推杆，再按 SF 档位控制。 */
class ThicknessControl final : public ModuleBase<ThicknessControl>,
                               public robot_wq::ScheduledWorkItem
{
public:
  ThicknessControl();
  ~ThicknessControl() override;
  static int task_spawn(int argc, char *argv[]);
  static int custom_command(int argc, char *argv[]);
  static int print_usage(const char *reason = nullptr);
  int print_status() override;
  void Run() override;

  /** 由工作队列调用一次，主机测试可在未调度的实例上直接调用。 */
  void update();

protected:
  bool stop_work_queue() override;

private:
  bool init();
  uint8_t sfPosition(uint64_t now) const;
  void updateMotion(uint8_t sf);
  void publishStatus(uint64_t now, uint8_t sf);

  ProbeLift _lift;
  uorb::Subscription<ActuatorArmed> _armSubscription;
  uorb::Subscription<FailsafeStatus> _failsafeSubscription;
  uorb::Subscription<ManualControlSwitches> _switchSubscription;
  uorb::Publication<ThicknessControlStatus> _publication;
  ActuatorArmed _arm{};
  FailsafeStatus _failsafe{};
  ManualControlSwitches _switches{};
  int32_t _sfSwitch{0};
  uint64_t _interruptedAt{0};
  uint64_t _readySince{0};
  uint8_t _phase{ThicknessControlStatus::WAITING_FOR_ARM};
  uint8_t _previousSf{ManualControlSwitches::POSITION_NONE};
  bool _armConsumed{true};
  bool _waitingForMiddle{false};
};
