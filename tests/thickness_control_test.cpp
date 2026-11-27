#include "robot/modules/thickness_control/ThicknessControl.hpp"
#include "robot/params/ParamManager.hpp"
#include "robot/work_queue/WorkQueueManager.hpp"
#include <probe_lift.h>
#include <assert.h>
#include <atomic>
#include <initializer_list>
#include <time.h>

namespace
{
using Message = ThicknessControlStatus;
using Switches = ManualControlSwitches;
constexpr uint8_t Up = Switches::POSITION_ON;
constexpr uint8_t Middle = Switches::POSITION_MIDDLE;
constexpr uint8_t Down = Switches::POSITION_OFF;
constexpr uint8_t Missing = Switches::POSITION_NONE;
std::atomic<uint64_t> timeUs{1000000};
std::atomic<bool> useRealClock{false};
bool in1 = false;
bool in2 = false;
bool workPressed = false;
unsigned initializations = 0;
unsigned switchInitializations = 0;
unsigned upWrites = 0;
unsigned downWrites = 0;
uint64_t pressAt = 0;
uint64_t releaseAt = 0;

void publishInputs(bool armed, uint8_t sf, bool lost, bool lockdown = false)
{
  const uint64_t now = os::Clock::nowMicroseconds();
  ActuatorArmed arm{};
  arm.timestamp = now;
  arm.armed = armed;
  arm.valid = !lost;
  arm.lockdown = lockdown;
  assert(actuatorArmedTopic().publish(arm));

  FailsafeStatus failsafe{};
  failsafe.timestamp = now;
  failsafe.active = lost;
  assert(failsafeStatusTopic().publish(failsafe));

  Switches switches{};
  switches.timestamp = switches.timestampSample = now;
  switches.switchCount = Switches::SWITCH_COUNT;
  switches.valid = !lost;
  switches.positions[5] = sf;
  assert(manualControlSwitchesTopic().publish(switches));
}

Message tick(ThicknessControl &control, bool armed, uint8_t sf,
             uint64_t elapsed = 5000, bool lost = false, bool lockdown = false)
{
  timeUs += elapsed;
  publishInputs(armed, sf, lost, lockdown);
  control.update();
  Message state{};
  assert(thicknessControlStatusTopic().copy(state));
  return state;
}

void braked() { assert(in1 && in2); }
void retracting() { assert(in1 && !in2); }
void extending() { assert(!in1 && in2); }

int bench(std::initializer_list<const char *> options)
{
  char name[] = "thickness_control";
  char test[] = "test";
  char *args[] = {name, test, nullptr, nullptr};
  int argc = 2;
  for (const char *option : options) { args[argc++] = const_cast<char *>(option); }
  return ThicknessControl::main(argc, args);
}
}

uint64_t os::Clock::nowMicroseconds()
{
  if (!useRealClock) { return timeUs.load(); }
  timespec now{};
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec * 1000000ULL + now.tv_nsec / 1000ULL;
}
void os::Clock::sleepMilliseconds(uint32_t milliseconds)
{
  if (useRealClock)
    {
      const timespec delay{static_cast<time_t>(milliseconds / 1000),
                           static_cast<long>(milliseconds % 1000) * 1000000L};
      nanosleep(&delay, nullptr);
    }
  else { timeUs += milliseconds * 1000ULL; }
}
int os::Task::terminate(int) { assert(false); return -1; }
extern "C" int board_probe_lift_initialize() { ++initializations; return 0; }
extern "C" int board_probe_lift_initialize_switch() { ++switchInitializations; return 0; }
extern "C" void board_probe_lift_write(bool a, bool b)
{
  in1 = a;
  in2 = b;
  if (a && !b) { ++upWrites; }
  if (!a && b) { ++downWrites; }
}
extern "C" bool board_probe_lift_work_pressed()
{
  const uint64_t now = os::Clock::nowMicroseconds();
  return workPressed || (pressAt != 0 && now >= pressAt && now < releaseAt);
}

int main()
{
  auto &params = ParamManager::instance();
  float resetTime = 0;
  assert(params.get("LIFT_RESET_TIME", resetTime) && resetTime == 10.0f);
  {
    ThicknessControl control;
    // Starting NSH while SA is already ON must wait for an observed OFF/ON.
    assert(tick(control, true, Down).phase == Message::WAITING_FOR_ARM);
    assert(initializations == 0);
    assert(tick(control, false, Down).phase == Message::WAITING_FOR_ARM);
    assert(tick(control, true, Down).phase == Message::HOMING);
    braked();
    assert(tick(control, true, Missing, 20000).phase == Message::HOMING);
    retracting();
    assert(tick(control, true, Up, 9999999).phase == Message::HOMING);
    retracting();
    assert(tick(control, true, Down, 1).phase == Message::WAITING_FOR_FIRST_WORK);
    braked();
    assert(initializations == 1);

    assert(tick(control, true, Down).phase == Message::WAITING_FOR_FIRST_WORK);
    assert(tick(control, true, Up).phase == Message::WAITING_FOR_FIRST_WORK);
    braked();
    assert(tick(control, true, Missing).phase == Message::WAITING_FOR_FIRST_WORK);
    assert(tick(control, true, Down).phase == Message::WAITING_FOR_FIRST_WORK);
    assert(tick(control, true, Middle).phase == Message::WAITING_FOR_FIRST_WORK);
    assert(tick(control, true, Down).phase == Message::MANUAL_CONTROL);
    tick(control, true, Down, 20000);
    extending();
    workPressed = true;
    assert(!tick(control, true, Down).measurementReady);
    extending();
    assert(!tick(control, true, Down, 45000).measurementReady);
    extending();
    const Message first_ready = tick(control, true, Down);
    assert(first_ready.measurementReady && first_ready.readySince == first_ready.timestamp);
    braked();

    workPressed = false;
    assert(tick(control, true, Down).readySince == first_ready.readySince);
    const Message released = tick(control, true, Down, 20000);
    assert(released.phase == Message::MANUAL_CONTROL && !released.measurementReady);
    assert(released.readySince == 0);
    assert(released.liftStatus == static_cast<uint8_t>(ProbeLift::Status::Stopped));
    assert(released.liftError == static_cast<uint8_t>(ProbeLift::Error::None));
    tick(control, true, Down, 50000);
    braked();
    tick(control, true, Middle);
    tick(control, true, Down);
    tick(control, true, Down, 20000);
    extending();
    workPressed = true;
    assert(!tick(control, true, Down).measurementReady);
    const Message second_ready = tick(control, true, Down, 50000);
    assert(second_ready.measurementReady && second_ready.readySince > first_ready.readySince);
    assert(!tick(control, true, Down, 5000, true).measurementReady);
    braked();

    assert(tick(control, true, Up).phase == Message::MANUAL_CONTROL);
    tick(control, true, Down, 20000);
    braked();
    tick(control, true, Middle);
    tick(control, true, Up);
    tick(control, true, Up, 20000);
    retracting();
    tick(control, true, Middle);
    braked();
    tick(control, true, Up);
    tick(control, true, Up, 20000);
    retracting();
    tick(control, true, Up, 5000, false, true);
    braked();
    assert(tick(control, true, Up).phase == Message::WAITING_FOR_ARM);
    assert(initializations == 1);
  }

  assert(params.set(params.find("LIFT_RESET_TIME"), 0.1f));
  constexpr uint8_t baselines[] = {Up, Middle};
  for (const uint8_t baseline : baselines)
    {
      workPressed = false;
      ThicknessControl control;
      tick(control, false, Missing);
      assert(tick(control, true, Missing).phase == Message::HOMING);
      tick(control, true, Down, 20000);
      retracting();
      assert(tick(control, true, Down, 5000, true).phase == Message::WAITING_FOR_ARM);
      braked();
      assert(tick(control, true, Up).phase == Message::WAITING_FOR_ARM);
      tick(control, false, Up);
      assert(tick(control, true, Up).phase == Message::HOMING);
      tick(control, true, Missing, 20000);
      assert(tick(control, true, baseline, 100000).phase == Message::WAITING_FOR_FIRST_WORK);
      braked();
      assert(tick(control, true, Down).phase == Message::MANUAL_CONTROL);
      tick(control, true, Down, 20000);
      extending();
      // No fresh arm/failsafe/switch input. A stale armed message cannot sustain motion.
      timeUs += 500001;
      control.update();
      braked();
    }

  // NSH lifecycle owns a scheduled item and publishes a safe state on stop.
  char name[] = "thickness_control";
  char start[] = "start";
  char stop[] = "stop";
  char *args[] = {name, start};
  useRealClock = true;
  assert(ThicknessControl::main(2, args) == 0);
  assert(ThicknessControl::is_running());
  assert(bench({"up", "100"}) != 0);
  args[1] = stop;
  assert(ThicknessControl::main(2, args) == 0);
  assert(!ThicknessControl::is_running());
  Message state{};
  assert(thicknessControlStatusTopic().copy(state) && !state.measurementReady);
  assert(robot_wq::SafetyQueue().Status().items == 0);
  useRealClock = false;
  const unsigned initCount = initializations;
  const unsigned upCount = upWrites;
  const unsigned downCount = downWrites;
  workPressed = false;
  pressAt = timeUs + 100000;
  releaseAt = timeUs + 200000;
  assert(bench({"switch"}) == 0);
  assert(switchInitializations == 1);
  assert(initializations == initCount && upWrites == upCount && downWrites == downCount);

  assert(bench({"down", "1001"}) != 0);
  assert(initializations == initCount);
  assert(params.set(params.find("LIFT_RESET_TIME"), 10.0f));
  const uint64_t upStart = timeUs.load();
  assert(bench({"up", "100"}) == 0);
  assert(timeUs - upStart == 100000 && upWrites > upCount && downWrites == downCount);
  braked();

  pressAt = timeUs + 50000;
  releaseAt = timeUs + 500000;
  assert(bench({"down", "200"}) == 0);
  assert(timeUs == pressAt + 50000 && downWrites > downCount);
  braked();
  const unsigned downWritesBeforeHeldSwitch = downWrites;
  assert(bench({"down", "100"}) == 0);
  assert(downWrites == downWritesBeforeHeldSwitch);
  braked();

  assert(robot_wq::SafetyQueue().Shutdown());
  puts("PASS: lift control arming, homing, SF transitions, failsafe and NSH lifecycle");
}
