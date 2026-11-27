#pragma once
#include "robot/os/Semaphore.hpp"
#include "robot/work_queue/WorkItem.hpp"
#include <pthread.h>
#include <stddef.h>

namespace robot_wq
{
struct QueueStatus
{
  const char *name;
  int priority;
  unsigned items;
  uint64_t runs;
  uint64_t maxRunUs;
};

/** Each queue has one worker and a coalesced notification for schedule changes. */
class WorkQueue
{
public:
  WorkQueue(const char *name, int priority, size_t stackSize);
  ~WorkQueue();
  WorkQueue(const WorkQueue &) = delete;
  WorkQueue &operator=(const WorkQueue &) = delete;
  bool Start();
  /** Only succeeds when no modules are attached. */
  bool Shutdown();
  bool Attach(WorkItem &item, uint64_t minimumIntervalUs);
  bool Detach(WorkItem &item, uint32_t timeoutMs);
  void Activate(WorkItem &item);
  void SetMinimumInterval(WorkItem &item, uint64_t intervalUs);
  void Schedule(WorkItem &item, uint64_t delayUs);
  void SetInterval(WorkItem &item, uint64_t intervalUs);
  void Clear(WorkItem &item);
  QueueStatus Status();

private:
  static void *Entry(void *argument);
  static int TaskEntry(int argc, char *argv[]);
  void Loop();
  // Both helpers are called with _mutex held. WaitForWork releases it while waiting.
  void WakeWorker();
  void WaitForWork(uint64_t monotonicDeadline);
  const char *_name;
  int _priority;
  size_t _stackSize;
  pthread_mutex_t _mutex{};
  os::Semaphore _wake;
  bool _wakePending{false};
#ifdef CBOARD_NUTTX
  pthread_cond_t _exitCondition{};
#endif
  pthread_t _thread{};
  int _taskId{-1};
  bool _workerExited{false};
  bool _started{false};
  bool _exit{false};
  WorkItem *_items{nullptr};
  uint64_t _runs{0};
  uint64_t _maxRunUs{0};
};
}
