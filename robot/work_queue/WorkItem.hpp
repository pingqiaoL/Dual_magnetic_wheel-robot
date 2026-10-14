#pragma once
#include <stdint.h>

namespace robot_wq
{
class WorkQueue;

/** A short, non-blocking computation owned by a module, not by the queue. */
class WorkItem
{
public:
  explicit WorkItem(WorkQueue &queue);
  virtual ~WorkItem();
  WorkItem(const WorkItem &) = delete;
  WorkItem &operator=(const WorkItem &) = delete;
  virtual void Run() = 0;
  void ScheduleNow();
  /** Permanently detach this registration; wait for an in-flight Run(). */
  bool StopAndWait(uint32_t timeoutMs = 5000);

protected:
  bool Attach(uint64_t minimumIntervalUs);
  WorkQueue &_queue;

private:
  friend class WorkQueue;
  WorkItem *_next{nullptr};
  bool _attached{false};
  bool _enabled{false};
  bool _stopping{false};
  bool _running{false};
  bool _pending{false};
  uint64_t _deadline{0};
  uint64_t _period{0};
  uint64_t _nextPeriodic{0};
  uint64_t _lastStart{0};
  uint64_t _minimumInterval{0};
};
}
