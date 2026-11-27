#pragma once

#include <stdint.h>

/** 单一工作队列调用。配置复位时间后 init 发起回收，调用方持续调用 update。 */
class ProbeLift
{
public:
  enum class Status { Stopped, MovingToWork, MovingToReset, WorkReady, ResetDone, Error };
  enum class Error { None, Initialization, InvalidParameter, Timeout };

  ProbeLift() = default;
  ~ProbeLift();
  ProbeLift(const ProbeLift &) = delete;
  ProbeLift &operator=(const ProbeLift &) = delete;

  bool init(bool requireResetTime = false);
  /** 配置 IO 和参数，保持制动。用于单独点动测试。 */
  bool initStopped();
  void work();
  void reset();
  void stop();
  void update();
  Status status() const { return _status; }
  Error error() const { return _error; }

private:
  enum class Direction : int8_t { Down = -1, Stop = 0, Up = 1 };
  void start(Direction direction);
  void updateReset(uint64_t now);
  void drive(Direction direction);
  void fail(Error error);

  bool _initialized{false};
  bool _downDriving{false};
  bool _timingSwitch{false};
  bool _resetDriving{false};
  int32_t _reverse{1};
  uint64_t _timeoutUs{0};
  uint64_t _resetTimeUs{0};
  uint64_t _pressStableUs{0};
  uint64_t _startedAt{0};
  uint64_t _switchSince{0};
  Status _status{Status::Stopped};
  Error _error{Error::None};
};
