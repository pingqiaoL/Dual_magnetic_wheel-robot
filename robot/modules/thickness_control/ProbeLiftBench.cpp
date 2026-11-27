#include "robot/modules/thickness_control/ThicknessControl.hpp"
#include <probe_lift.h>
#include <stdlib.h>

namespace
{
constexpr uint64_t SwitchTestUs = 10000000;

int testSwitch()
{
  if (board_probe_lift_initialize_switch() < 0)
    {
      fprintf(stderr, "C5 input initialization failed\n");
      return -1;
    }

  printf("C5 switch test (10 s): press and release the switch.\n");
  const uint64_t start = os::Clock::nowMicroseconds();
  bool pressed = board_probe_lift_work_pressed();
  bool sawPress = false;
  bool sawRelease = false;
  printf("C5: %s\n", pressed ? "pressed (LOW)" : "released (HIGH)");
  fflush(stdout);

  while (os::Clock::nowMicroseconds() - start < SwitchTestUs)
    {
      const bool sample = board_probe_lift_work_pressed();
      if (sample != pressed)
        {
          pressed = sample;
          sawPress |= pressed;
          sawRelease |= !pressed;
          printf("C5: %s\n", pressed ? "pressed (LOW)" : "released (HIGH)");
          fflush(stdout);
        }
      os::Clock::sleepMilliseconds(10);
    }

  if (sawPress && sawRelease)
    {
      printf("PASS: C5 press and release detected.\n");
      return 0;
    }
  fprintf(stderr, "INCOMPLETE: did not see both C5 transitions.\n");
  return -1;
}

int testMove(bool up, unsigned long durationMs)
{
  ProbeLift lift;
  if (!lift.initStopped())
    {
      fprintf(stderr, "lift initialization failed (error %u)\n",
              static_cast<unsigned>(lift.error()));
      return -1;
    }

  printf("Testing %s for up to %lu ms.\n", up ? "up" : "down", durationMs);
  fflush(stdout);
  const uint64_t start = os::Clock::nowMicroseconds();
  if (up) { lift.reset(); }
  else { lift.work(); }

  const uint64_t durationUs = durationMs * 1000ULL;
  while (os::Clock::nowMicroseconds() - start < durationUs &&
         (lift.status() == ProbeLift::Status::MovingToReset ||
          lift.status() == ProbeLift::Status::MovingToWork))
    {
      lift.update();
      if (lift.status() == ProbeLift::Status::MovingToReset ||
          lift.status() == ProbeLift::Status::MovingToWork)
        { os::Clock::sleepMilliseconds(5); }
    }

  const auto result = lift.status();
  lift.stop();
  if (result == ProbeLift::Status::Error)
    {
      fprintf(stderr, "Stopped: lift error %u.\n", static_cast<unsigned>(lift.error()));
      return -1;
    }
  if (result == ProbeLift::Status::WorkReady)
    { printf("Stopped: C5 work switch confirmed.\n"); }
  else if (result == ProbeLift::Status::ResetDone)
    { printf("Stopped: configured reset time elapsed.\n"); }
  else
    { printf("Stopped: test time elapsed.\n"); }
  return 0;
}
}

int ThicknessControl::custom_command(int argc, char *argv[])
{
  if (argc < 2 || strcmp(argv[0], "test") != 0) { return print_usage("unknown command"); }
  // ModuleBase 在本函数执行期间持有生命周期锁。
  if (is_running()) { return print_usage("stop thickness_control before bench testing"); }
  if (argc == 2 && strcmp(argv[1], "switch") == 0) { return testSwitch(); }
  if (argc != 3 || (strcmp(argv[1], "up") != 0 && strcmp(argv[1], "down") != 0))
    { return print_usage("invalid bench test"); }

  char *end = nullptr;
  const unsigned long durationMs = strtoul(argv[2], &end, 10);
  if (argv[2][0] < '0' || argv[2][0] > '9' || *end != '\0' ||
      durationMs < 50 || durationMs > 1000)
    { return print_usage("duration must be between 50 and 1000 ms"); }
  return testMove(strcmp(argv[1], "up") == 0, durationMs);
}
