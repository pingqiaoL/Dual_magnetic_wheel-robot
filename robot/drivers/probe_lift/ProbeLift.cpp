#include "robot/drivers/probe_lift/ProbeLift.hpp"
#include "robot/os/Clock.hpp"
#include "robot/params/ParamManager.hpp"
#include <probe_lift.h>
#include <math.h>

namespace
{
constexpr uint64_t ReleaseStableUs = 20000;
constexpr uint64_t DirectionPauseUs = 20000;
}

ProbeLift::~ProbeLift()
{
  stop();
}

bool ProbeLift::init(bool requireResetTime)
{
  if (!initStopped()) { return false; }
  if (_resetTimeUs != 0) { reset(); }
  else if (requireResetTime)
    {
      fail(Error::InvalidParameter);
      return false;
    }
  return true;
}

bool ProbeLift::initStopped()
{
  stop();
  _initialized = false;
  _error = Error::None;
  if (board_probe_lift_initialize() < 0)
    {
      fail(Error::Initialization);
      return false;
    }
  _initialized = true;
  drive(Direction::Stop);

  float timeout = 0.0f;
  float resetTime = 0.0f;
  int32_t pressStableMs = 0;
  ParamManager &params = ParamManager::instance();
  if (!params.get("LIFT_REV", _reverse) ||
      !params.get("LIFT_TIMEOUT", timeout) ||
      !params.get("LIFT_RESET_TIME", resetTime) ||
      !params.get("LIFT_C4_STABLE", pressStableMs) ||
      pressStableMs < 1 || pressStableMs > 1000 ||
      (_reverse != 1 && _reverse != -1) ||
      !isfinite(timeout) || timeout < 0.1f || timeout > 120.0f ||
      !isfinite(resetTime) || resetTime > 120.0f ||
      (resetTime != 0.0f && resetTime < 0.1f))
    {
      fail(Error::InvalidParameter);
      return false;
    }
  _timeoutUs = static_cast<uint64_t>(timeout * 1000000.0f);
  _resetTimeUs = static_cast<uint64_t>(resetTime * 1000000.0f);
  _pressStableUs = static_cast<uint64_t>(pressStableMs) * 1000;
  _status = Status::Stopped;
  return true;
}

void ProbeLift::drive(Direction direction)
{
  const int output = static_cast<int>(direction) * _reverse;
  // 11 为制动，10 / 01 为两个运动极性。
  board_probe_lift_write(output >= 0, output <= 0);
}

void ProbeLift::stop()
{
  if (_initialized) { drive(Direction::Stop); }
  // 故障保留至重新 init，避免重复 work 指令清除故障并自动重启。
  if (_status != Status::Error) { _status = Status::Stopped; }
  _downDriving = false;
  _timingSwitch = false;
  _resetDriving = false;
}

void ProbeLift::fail(Error error)
{
  stop();
  _error = error;
  _status = Status::Error;
}

void ProbeLift::work() { start(Direction::Down); }
void ProbeLift::reset() { start(Direction::Up); }

void ProbeLift::start(Direction direction)
{
  if (!_initialized)
    {
      fail(Error::Initialization);
      return;
    }
  if (_status == Status::Error) { return; }
  const Status moving = direction == Direction::Up ? Status::MovingToReset : Status::MovingToWork;
  if (direction == Direction::Up && _resetTimeUs == 0)
    {
      fail(Error::InvalidParameter);
      return;
    }
  const Status completed = direction == Direction::Up ? Status::ResetDone : Status::WorkReady;
  if (_status == moving || _status == completed)
    {
      update();
      return;
    }
  stop();
  _status = moving;
  _startedAt = os::Clock::nowMicroseconds();
  update();
}

void ProbeLift::updateReset(uint64_t now)
{
  if (!_resetDriving)
    {
      if (now - _startedAt < DirectionPauseUs) { return; }
      drive(Direction::Up);
      _startedAt = now;
      _resetDriving = true;
      return;
    }
  if (now - _startedAt >= _resetTimeUs)
    {
      stop();
      _status = Status::ResetDone;
    }
}

void ProbeLift::update()
{
  const uint64_t now = os::Clock::nowMicroseconds();
  if (_status == Status::MovingToReset)
    {
      updateReset(now);
      return;
    }
  if (_status != Status::MovingToWork && _status != Status::WorkReady) { return; }

  const bool pressed = board_probe_lift_work_pressed();
  if (_status == Status::WorkReady)
    {
      if (pressed) { _timingSwitch = false; }
      else if (!_timingSwitch)
        {
          _switchSince = now;
          _timingSwitch = true;
        }
      else if (now - _switchSince >= ReleaseStableUs) { stop(); }
      return;
    }

  if (now - _startedAt >= _timeoutUs)
    {
      fail(Error::Timeout);
      return;
    }
  if (pressed)
    {
      if (!_timingSwitch)
        {
          _switchSince = now;
          _timingSwitch = true;
        }
      if (now - _switchSince >= _pressStableUs)
        {
          stop();
          _status = Status::WorkReady;
        }
      return;
    }
  _timingSwitch = false;
  // 工作开关已按下时先确认再决定是否报告就绪。已在下压时，输出持续到确认完成。
  if (!_downDriving && now - _startedAt >= DirectionPauseUs)
    {
      drive(Direction::Down);
      _downDriving = true;
    }
}
