#include "robot/drivers/probe_lift/ProbeLift.hpp"
#include "robot/os/Clock.hpp"
#include "robot/params/ParamManager.hpp"
#include <probe_lift.h>
#include <assert.h>
#include <initializer_list>
#include <stdio.h>

namespace
{
uint64_t now = 0;
bool workSwitch = false;
bool in1 = false;
bool in2 = false;
bool gpioFailure = false;
using Status = ProbeLift::Status;
using Error = ProbeLift::Error;

class Storage : public ParamStorage
{
public:
  bool load(ParamStorageSnapshot &) override { return false; }
  bool save(const ParamStorageSnapshot &) override { return true; }
};

void tick(ProbeLift &lift, uint64_t elapsed = 20000)
{
  now += elapsed;
  lift.update();
}

void stopped() { assert(in1 && in2); }
void drivingDown(int reverse) { assert(in1 == (reverse == -1) && in2 == (reverse == 1)); }
}

uint64_t os::Clock::nowMicroseconds() { return now; }
extern "C" int board_probe_lift_initialize() { in1 = in2 = false; return gpioFailure ? -1 : 0; }
extern "C" void board_probe_lift_write(bool a, bool b) { in1 = a; in2 = b; }
extern "C" bool board_probe_lift_work_pressed() { return workSwitch; }

int main()
{
  Storage storage;
  auto &params = ParamManager::instance();
  assert(params.initialize(storage));
  int32_t pressStableMs = 0;
  assert(params.get("LIFT_C4_STABLE", pressStableMs) && pressStableMs == 50);
  assert(params.set(params.find("LIFT_TIMEOUT"), 0.2f));
  assert(params.set(params.find("LIFT_RESET_TIME"), 0.0f));
  {
    ProbeLift uninitialized;
    uninitialized.work();
    assert(uninitialized.error() == Error::Initialization);
  }
  {
    ProbeLift lift;
    assert(lift.init()); // 未标定复位时间时可先验证工作开关。
    assert(lift.status() == Status::Stopped);
    stopped();
    workSwitch = true;
    lift.work();
    stopped();
    tick(lift, 49999);
    assert(lift.status() == Status::MovingToWork);
    stopped();
    tick(lift, 1);
    assert(lift.status() == Status::WorkReady);
    stopped();
    lift.reset();
    assert(lift.error() == Error::InvalidParameter);
    stopped();
    assert(!lift.init(true));
    assert(lift.error() == Error::InvalidParameter);
  }
  assert(params.set(params.find("LIFT_RESET_TIME"), 0.5f));
  for (int reverse : {1, -1})
    {
      assert(params.set(params.find("LIFT_REV"), reverse));
      workSwitch = true; // 允许从工作开关已按下的位置回收。
      ProbeLift lift;
      assert(lift.init());
      assert(lift.status() == Status::MovingToReset);
      stopped();
      tick(lift, 80000); // 复位计时以首次施加电压为起点。
      assert(in1 == (reverse == 1) && in2 == (reverse == -1));
      workSwitch = false;
      tick(lift, 499999);
      assert(lift.status() == Status::MovingToReset);
      assert(in1 == (reverse == 1) && in2 == (reverse == -1));
      lift.reset(); // 重复命令不重置计时。
      tick(lift, 1);
      assert(lift.status() == Status::ResetDone);
      stopped();
      lift.reset();
      assert(lift.status() == Status::ResetDone);
      stopped();
      lift.work();
      stopped();
      tick(lift);
      drivingDown(reverse);
      workSwitch = true;
      lift.update();
      drivingDown(reverse);
      workSwitch = false;
      tick(lift, 5000);
      drivingDown(reverse);
      workSwitch = true;
      tick(lift, 5000);
      tick(lift, 49999);
      assert(lift.status() == Status::MovingToWork);
      drivingDown(reverse);
      tick(lift, 1);
      assert(lift.status() == Status::WorkReady);
      stopped();
      lift.work();
      assert(lift.status() == Status::WorkReady);

      workSwitch = false;
      lift.update();
      tick(lift, 19999);
      assert(lift.status() == Status::WorkReady);
      workSwitch = true;
      tick(lift, 1);
      assert(lift.status() == Status::WorkReady);
      workSwitch = false;
      lift.update();
      tick(lift);
      assert(lift.status() == Status::Stopped && lift.error() == Error::None);
      stopped();
      lift.work();
      tick(lift);
      assert(lift.status() == Status::MovingToWork);
      drivingDown(reverse);
      lift.stop();

      assert(lift.init());
      tick(lift);
      lift.stop();
      tick(lift, 300000);
      assert(lift.status() == Status::Stopped);
      stopped();
      lift.work();
      tick(lift);
      lift.reset();  // 直接换向时先制动，经过间隔才施加另一极性。
      stopped();
      tick(lift, 10000);
      stopped();
      tick(lift, 10000);
      assert(in1 == (reverse == 1));
      tick(lift, 500000);
      assert(lift.status() == Status::ResetDone);
      stopped();
    }

  ProbeLift lift;
  workSwitch = false;
  assert(lift.init());
  tick(lift);
  lift.work();
  tick(lift, 190000);
  workSwitch = true;
  lift.update();
  drivingDown(-1);
  tick(lift, 20000); // 开关没有在下压期限内确认稳定。
  assert(lift.error() == Error::Timeout);
  stopped();

  assert(lift.init());
  workSwitch = false;
  lift.work();
  tick(lift, 199999);
  assert(lift.status() == Status::MovingToWork);
  lift.work();
  tick(lift, 1);
  assert(lift.error() == Error::Timeout);
  stopped();

  assert(params.set(params.find("LIFT_REV"), 0));
  assert(!lift.init());
  assert(lift.error() == Error::InvalidParameter);
  stopped();
  assert(params.set(params.find("LIFT_REV"), 1));
  assert(params.set(params.find("LIFT_TIMEOUT"), -1.0f));
  assert(!lift.init());
  stopped();
  assert(params.set(params.find("LIFT_TIMEOUT"), 10.0f));
  assert(params.set(params.find("LIFT_RESET_TIME"), -1.0f));
  assert(!lift.init());
  assert(lift.error() == Error::InvalidParameter);
  stopped();
  assert(params.set(params.find("LIFT_RESET_TIME"), 0.5f));
  const auto stableParam = params.find("LIFT_C4_STABLE");
  assert(params.set(stableParam, 0));
  assert(!lift.init());
  assert(lift.error() == Error::InvalidParameter);
  stopped();
  assert(params.set(stableParam, 75));
  workSwitch = false;
  assert(lift.initStopped());
  lift.work();
  tick(lift);
  workSwitch = true;
  lift.update();
  tick(lift, 74999);
  assert(lift.status() == Status::MovingToWork);
  drivingDown(1);
  tick(lift, 1);
  assert(lift.status() == Status::WorkReady);
  stopped();

  gpioFailure = true;
  assert(!lift.init());
  assert(lift.error() == Error::Initialization);
  gpioFailure = false;
  {
    ProbeLift owned;
    assert(owned.init());
    tick(owned);
  }
  stopped(); // 析构停止输出。
  printf("PASS: timed reset, lift polarity, work switch, stop, timeout and faults\n");
}
