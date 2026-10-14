#include "robot/modules/RcUpdate.hpp"
#include "robot/modules/Command.hpp"
#include "robot/modules/Failsafe.hpp"
#include "robot/modules/RobotControl.hpp"
#include "robot/modules/ControlAllocator.hpp"
#include "robot/params/ParamManager.hpp"
#include "robot/os/Clock.hpp"
#include <assert.h>
#include <math.h>
#include <stdio.h>

// Fail if a migrated module tries to start or kill an independent OS task.
int os::Task::spawn(const char *, int, size_t, Entry, int, char *[])
{ assert(false); return -1; }
int os::Task::terminate(int) { assert(false); return -1; }

template<class T>
static int command(const char *action)
{
  char name[] = "module";
  char *argv[] = {name, const_cast<char *>(action)};
  return T::main(2, argv);
}
template<class Predicate>
static void waitFor(Predicate predicate, uint32_t timeoutMs = 1500)
{
  const uint64_t deadline = os::Clock::nowMicroseconds() + timeoutMs * 1000ULL;
  while (!predicate() && os::Clock::nowMicroseconds() < deadline)
    { os::Clock::sleepMilliseconds(2); }
  assert(predicate());
}
int main()
{
  ParamManager &params = ParamManager::instance();
  assert(params.set(params.find("FS_RC_TMO"), 0.1f));
  assert(robot_wq::Initialize());
  assert(command<RcUpdate>("start") == 0);
  assert(command<Failsafe>("start") == 0);
  assert(command<RobotControl>("start") == 0);
  assert(command<ControlAllocator>("start") == 0);
  assert(command<Command>("start") == 0);
  assert(robot_wq::SafetyQueue().Status().items == 2);
  assert(robot_wq::ControlQueue().Status().items == 3);
  assert(command<ControlAllocator>("start") != 0);

  // Feed real input_rc data through all five queued modules.
  InputRc input{};
  input.channelCount = 16;
  for (unsigned i = 0; i < 16; ++i) { input.values[i] = 1500; }
  input.values[2] = 1700; // RC_MAP_THROTTLE=3 (one-based)
  input.values[3] = 1400; // RC_MAP_YAW=4 (one-based)
  input.values[4] = 1000; // safety switch OFF
  for (unsigned frame = 0; frame < 8; ++frame)
    {
      input.timestamp = input.timestampLastSignal = os::Clock::nowMicroseconds();
      assert(inputRcTopic().publish(input));
      os::Clock::sleepMilliseconds(15);
    }
  ActuatorMotors motors{};
  waitFor([&] { return actuatorMotorsTopic().copy(motors) && motors.valid &&
                       motors.control[0] > 0.1f; });
  input.values[4] = 2000; // OFF has been observed; now ON arms.
  input.timestamp = input.timestampLastSignal = os::Clock::nowMicroseconds();
  assert(inputRcTopic().publish(input));
  ActuatorArmed armed{};
  waitFor([&] { return actuatorArmedTopic().copy(armed) && armed.armed; });

  // No further input: periodic fallback still detects loss and publishes zero.
  RobotControlSetpoint setpoint{};
  waitFor([&] { return robotControlSetpointTopic().copy(setpoint) &&
                       setpoint.valid && setpoint.failsafe && setpoint.speed == 0.0f; });
  assert(command<Command>("stop") == 0);
  assert(actuatorArmedTopic().copy(armed) && !armed.armed);
  assert(command<ControlAllocator>("stop") == 0);
  assert(actuatorMotorsTopic().copy(motors) && !motors.valid);
  assert(command<RobotControl>("stop") == 0);
  assert(robotControlSetpointTopic().copy(setpoint) && !setpoint.valid);
  assert(command<Failsafe>("stop") == 0);
  FailsafeStatus failsafe{};
  assert(failsafeStatusTopic().copy(failsafe) && failsafe.active);
  assert(command<RcUpdate>("stop") == 0);
  ManualControl manual{};
  assert(manualControlTopic().copy(manual) && !manual.valid);
  assert(command<Command>("stop") == 0); // repeated stop is safe

  // Stop/start must unregister old callbacks and release objects.
  for (unsigned i = 0; i < 10; ++i)
    {
      assert(command<Command>("start") == 0);
      assert(command<Command>("stop") == 0);
    }
  assert(robot_wq::SafetyQueue().Status().items == 0);
  assert(robot_wq::ControlQueue().Status().items == 0);
  assert(robot_wq::SafetyQueue().Shutdown());
  assert(robot_wq::ControlQueue().Shutdown());
  puts("five queued modules: input, arming, loss, safe stop and restart passed");
}
