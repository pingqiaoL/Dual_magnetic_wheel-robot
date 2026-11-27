#ifdef main
#undef main
#endif

#include "robot/modules/thickness_control/ThicknessControl.hpp"
#include "robot/params/ParamManager.hpp"
#include "robot/work_queue/WorkQueueConfigurations.hpp"

namespace
{
using Switches = ManualControlSwitches;
using Status = ThicknessControlStatus;
constexpr uint64_t ControlIntervalUs = 5000;
constexpr uint64_t InputTimeoutUs = 500000;

bool fresh(uint64_t timestamp, uint64_t now)
{
  return timestamp != 0 && timestamp <= now && now - timestamp <= InputTimeoutUs;
}
}

ThicknessControl::ThicknessControl()
  : ScheduledWorkItem("thickness_control", robot_wq::wq_configurations::safety),
    _armSubscription(actuatorArmedTopic()),
    _failsafeSubscription(failsafeStatusTopic()),
    _switchSubscription(manualControlSwitchesTopic()),
    _publication(thicknessControlStatusTopic())
{
  (void)ParamManager::instance().get("LIFT_SF_SW", _sfSwitch);
}

ThicknessControl::~ThicknessControl()
{
  ScheduleClear();
  while (!StopAndWait()) {}
  _lift.stop();
}

int ThicknessControl::task_spawn(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  auto *instance = new ThicknessControl();
  if (instance == nullptr) { return -1; }
  if (!instance->init())
    {
      delete instance;
      return -1;
    }
  __atomic_store_n(&_object, instance, __ATOMIC_RELEASE);
  __atomic_store_n(&_taskId, WorkQueueTaskId, __ATOMIC_RELEASE);
  return 0;
}

bool ThicknessControl::init()
{
  if (!initialized()) { return false; }
  if (_sfSwitch < 1 || _sfSwitch > Switches::SWITCH_COUNT)
    {
      fprintf(stderr, "LIFT_SF_SW must be between 1 and 12\n");
      return false;
    }
  publishStatus(os::Clock::nowMicroseconds(), Switches::POSITION_NONE);
  ActivateSchedule();
  ScheduleOnInterval(ControlIntervalUs);
  ScheduleNow();
  return true;
}

void ThicknessControl::Run()
{
  update();
}

bool ThicknessControl::stop_work_queue()
{
  ScheduleClear();
  if (!StopAndWait()) { return false; }
  _lift.stop();
  _phase = Status::WAITING_FOR_ARM;
  publishStatus(os::Clock::nowMicroseconds(), Switches::POSITION_NONE);
  return true;
}

uint8_t ThicknessControl::sfPosition(uint64_t now) const
{
  if (!_switches.valid || !fresh(_switches.timestampSample, now) ||
      _sfSwitch < 1 || _sfSwitch > Switches::SWITCH_COUNT || _sfSwitch > _switches.switchCount)
    { return Switches::POSITION_NONE; }

  const uint8_t sf = _switches.positions[_sfSwitch - 1];
  return sf >= Switches::POSITION_ON && sf <= Switches::POSITION_OFF
         ? sf : Switches::POSITION_NONE;
}

void ThicknessControl::update()
{
  (void)_armSubscription.update(_arm);
  (void)_failsafeSubscription.update(_failsafe);
  (void)_switchSubscription.update(_switches);
  const uint64_t now = os::Clock::nowMicroseconds();
  const bool armFresh = fresh(_arm.timestamp, now);

  if (armFresh && _arm.lockdown)
    {
      // 清除手动锁定时仍需一次明确的 SA 关、开操作。
      _armConsumed = true;
      _phase = Status::WAITING_FOR_ARM;
    }
  else if (armFresh && _arm.valid && !_arm.armed)
    {
      _armConsumed = false;
      _phase = Status::WAITING_FOR_ARM;
    }

  const bool motionAllowed = armFresh && _arm.valid && _arm.armed && !_arm.lockdown &&
                             fresh(_failsafe.timestamp, now) && !_failsafe.active;
  const uint8_t sf = sfPosition(now);
  if (!motionAllowed)
    {
      _lift.stop();
      _interruptedAt = now;
      _previousSf = Switches::POSITION_NONE;
      _waitingForMiddle = true;
      // 失联可能保留 armed，已开始的回零必须等下一次真实解锁。
      if (_phase == Status::HOMING) { _phase = Status::WAITING_FOR_ARM; }
    }
  else
    {
      // 恢复时等待失联之后收到的遥控样本，再建立档位基准。
      updateMotion(_switches.timestampSample > _interruptedAt ? sf : Switches::POSITION_NONE);
      if (_phase != Status::WAITING_FOR_ARM &&
          _lift.status() == ProbeLift::Status::Error)
        { _phase = Status::FAULT; }
    }
  publishStatus(now, sf);
}

void ThicknessControl::updateMotion(uint8_t sf)
{
  switch (_phase)
    {
      case Status::WAITING_FOR_ARM:
        if (!_armConsumed)
          {
            _armConsumed = true;
            _waitingForMiddle = false;
            _phase = _lift.init(true) ? Status::HOMING : Status::FAULT;
          }
        break;

      case Status::HOMING:
        _lift.update();
        if (_lift.status() == ProbeLift::Status::ResetDone)
          {
            _previousSf = sf;
            _phase = Status::WAITING_FOR_FIRST_WORK;
          }
        break;

      case Status::WAITING_FOR_FIRST_WORK:
        if (sf == Switches::POSITION_OFF &&
            (_previousSf == Switches::POSITION_ON ||
             _previousSf == Switches::POSITION_MIDDLE))
          {
            _lift.work();
            _phase = Status::MANUAL_CONTROL;
          }
        _previousSf = sf;
        break;

      case Status::MANUAL_CONTROL:
        if (sf == Switches::POSITION_NONE)
          {
            _lift.stop();
            _waitingForMiddle = true;
          }
        else if (sf == Switches::POSITION_MIDDLE)
          {
            _lift.stop();
            _waitingForMiddle = false;
          }
        else if (!_waitingForMiddle && sf != _previousSf)
          {
            if (sf == Switches::POSITION_ON) { _lift.reset(); }
            else { _lift.work(); }
          }
        _previousSf = sf;
        _lift.update();
        break;

      case Status::FAULT:
        break;
    }
}

void ThicknessControl::publishStatus(uint64_t now, uint8_t sf)
{
  Status status{};
  status.timestamp = now;
  status.phase = _phase;
  status.sfPosition = sf;
  status.liftStatus = static_cast<uint8_t>(_lift.status());
  status.liftError = static_cast<uint8_t>(_lift.error());
  status.measurementReady = _phase == Status::MANUAL_CONTROL &&
                            _lift.status() == ProbeLift::Status::WorkReady;
  if (!status.measurementReady) { _readySince = 0; }
  else if (_readySince == 0) { _readySince = now; }
  status.readySince = _readySince;
  (void)_publication.publish(status);
}

int ThicknessControl::print_usage(const char *reason)
{
  if (reason != nullptr) { fprintf(stderr, "%s\n", reason); }
  printf("usage: thickness_control {start|stop|status}\n"
         "       thickness_control test switch\n"
         "       thickness_control test {up|down} <50..1000 ms>\n");
  return reason == nullptr ? 0 : -1;
}

int ThicknessControl::print_status()
{
  Status status{};
  if (!thicknessControlStatusTopic().copy(status)) { return -1; }
  const char *const phases[] = {"waiting_for_arm", "homing", "waiting_for_first_work",
                                "manual_control", "fault"};
  const char *const positions[] = {"invalid", "up", "middle", "down"};
  const char *const liftStates[] = {"stopped", "moving_to_work", "moving_to_reset",
                                    "work_ready", "reset_done", "error"};
  const char *const errors[] = {"none", "initialization", "invalid_parameter", "timeout"};
  printf("SF switch: %ld (%s)\nphase: %s\nlift: %s\nerror: %s\nmeasurement ready: %s\n",
         static_cast<long>(_sfSwitch), positions[status.sfPosition], phases[status.phase],
         liftStates[status.liftStatus], errors[status.liftError],
         status.measurementReady ? "yes" : "no");
  return 0;
}

extern "C" int thickness_control_main(int argc, char *argv[])
{
  return ThicknessControl::main(argc, argv);
}
