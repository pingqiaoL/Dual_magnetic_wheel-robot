#include "robot/work_queue/WorkQueue.hpp"
#include "robot/work_queue/ScheduledWorkItem.hpp"
#include "robot/os/Clock.hpp"
#include "robot/os/Task.hpp"
#include <assert.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace robot_wq
{
WorkItem::WorkItem(WorkQueue &queue) : _queue(queue) {}
WorkItem::~WorkItem()
{
  // Derived state must remain alive until callbacks and execution have stopped.
  assert(!_attached);
}
bool WorkItem::Attach(uint64_t minimumIntervalUs)
{ return _queue.Attach(*this, minimumIntervalUs); }
void WorkItem::ScheduleNow() { _queue.Schedule(*this, 0); }
bool WorkItem::StopAndWait(uint32_t timeoutMs)
{ return _queue.Detach(*this, timeoutMs); }
bool ScheduledWorkItem::InitSchedule(uint64_t minimumIntervalUs)
{ return Attach(minimumIntervalUs); }
void ScheduledWorkItem::ScheduleDelayed(uint64_t delayUs)
{ _queue.Schedule(*this, delayUs); }
void ScheduledWorkItem::ScheduleOnInterval(uint64_t intervalUs)
{ _queue.SetInterval(*this, intervalUs); }
void ScheduledWorkItem::ScheduleClear() { _queue.Clear(*this); }

WorkQueue::WorkQueue(const char *name, int priority, size_t stackSize)
  : _name(name), _priority(priority), _stackSize(stackSize)
{
  pthread_mutex_init(&_mutex, nullptr);
  pthread_condattr_t attributes;
  pthread_condattr_init(&attributes);
  if (pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) == 0)
    { _conditionClock = CLOCK_MONOTONIC; }
  pthread_cond_init(&_condition, &attributes);
  pthread_condattr_destroy(&attributes);
}

WorkQueue::~WorkQueue()
{
  const bool stopped = Shutdown();
  assert(stopped);
  (void)stopped;
  pthread_cond_destroy(&_condition);
  pthread_mutex_destroy(&_mutex);
}

bool WorkQueue::Start()
{
  pthread_mutex_lock(&_mutex);
  if (_started) { pthread_mutex_unlock(&_mutex); return true; }
  _exit = false;
  _workerExited = false;
#ifdef CBOARD_NUTTX
  // A dedicated task group keeps queues alive when 'robot stop' ends the
  // runtime task. A pthread owned by robot_core would share its task group.
  char context[2 * sizeof(uintptr_t) + 1];
  snprintf(context, sizeof(context), "%llx",
           static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(this)));
  char *arguments[] = {context, nullptr};
  _taskId = os::Task::spawn(_name, _priority, _stackSize, TaskEntry, 1, arguments);
  const int result = _taskId < 0 ? -1 : 0;
#else
  pthread_attr_t attributes;
  int result = pthread_attr_init(&attributes);
  if (result != 0) { pthread_mutex_unlock(&_mutex); return false; }
  if (result == 0) { result = pthread_create(&_thread, &attributes, Entry, this); }
  pthread_attr_destroy(&attributes);
#endif
  _started = result == 0;
  pthread_mutex_unlock(&_mutex);
  return result == 0;
}

bool WorkQueue::Shutdown()
{
  pthread_mutex_lock(&_mutex);
  if (_items != nullptr) { pthread_mutex_unlock(&_mutex); return false; }
  if (!_started) { pthread_mutex_unlock(&_mutex); return true; }
  _exit = true;
  pthread_cond_broadcast(&_condition);
#ifdef CBOARD_NUTTX
  while (!_workerExited) { pthread_cond_wait(&_condition, &_mutex); }
  _started = false;
  _taskId = -1;
  pthread_mutex_unlock(&_mutex);
  return true;
#else
  pthread_mutex_unlock(&_mutex);
  const int result = pthread_join(_thread, nullptr);
  pthread_mutex_lock(&_mutex);
  if (result == 0) { _started = false; }
  pthread_mutex_unlock(&_mutex);
  return result == 0;
#endif
}

bool WorkQueue::Attach(WorkItem &item, uint64_t minimumIntervalUs)
{
  if (!Start()) { return false; }
  pthread_mutex_lock(&_mutex);
  if (item._attached || _exit)
    { pthread_mutex_unlock(&_mutex); return false; }
  item._minimumInterval = minimumIntervalUs;
  item._lastStart = 0;
  item._pending = false;
  item._period = 0;
  item._attached = true;
  item._enabled = false; // Activation follows callback registration and initialization.
  item._stopping = false;
  // Insert at the tail so startup order resolves simultaneous deadlines.
  WorkItem **tail = &_items;
  while (*tail != nullptr) { tail = &(*tail)->_next; }
  *tail = &item;
  item._next = nullptr;
  pthread_mutex_unlock(&_mutex);
  return true;
}

bool WorkQueue::Detach(WorkItem &item, uint32_t timeoutMs)
{
  pthread_mutex_lock(&_mutex);
  if (!item._attached) { pthread_mutex_unlock(&_mutex); return true; }
  // A running item cannot synchronously wait for itself.
#ifdef CBOARD_NUTTX
  const bool self = _started && getpid() == _taskId;
#else
  const bool self = _started && pthread_equal(pthread_self(), _thread);
#endif
  if (self)
    { pthread_mutex_unlock(&_mutex); return false; }
  item._enabled = false;
  item._stopping = true;
  item._pending = false;
  item._period = 0;
  pthread_cond_broadcast(&_condition);
  const uint64_t deadline = os::Clock::nowMicroseconds() +
                            static_cast<uint64_t>(timeoutMs) * 1000ULL;
  while (item._running)
    {
      if (os::Clock::nowMicroseconds() >= deadline)
        { pthread_mutex_unlock(&_mutex); return false; }
      WaitUntil(deadline);
    }
  WorkItem **cursor = &_items;
  while (*cursor != nullptr && *cursor != &item) { cursor = &(*cursor)->_next; }
  if (*cursor == &item) { *cursor = item._next; }
  item._next = nullptr;
  item._attached = false;
  pthread_mutex_unlock(&_mutex);
  return true;
}

void WorkQueue::Schedule(WorkItem &item, uint64_t delayUs)
{
  pthread_mutex_lock(&_mutex);
  if (item._enabled)
    {
      uint64_t deadline = os::Clock::nowMicroseconds() + delayUs;
      const uint64_t earliest = item._lastStart + item._minimumInterval;
      if (item._lastStart != 0 && deadline < earliest) { deadline = earliest; }
      if (!item._pending || deadline < item._deadline) { item._deadline = deadline; }
      item._pending = true;
      pthread_cond_broadcast(&_condition);
    }
  pthread_mutex_unlock(&_mutex);
}

void WorkQueue::SetInterval(WorkItem &item, uint64_t intervalUs)
{
  pthread_mutex_lock(&_mutex);
  if (item._attached && !item._stopping)
    {
      item._enabled = true;
      item._period = intervalUs;
      item._nextPeriodic = os::Clock::nowMicroseconds() + intervalUs;
      pthread_cond_broadcast(&_condition);
    }
  pthread_mutex_unlock(&_mutex);
}

void WorkQueue::Clear(WorkItem &item)
{
  pthread_mutex_lock(&_mutex);
  item._pending = false;
  item._period = 0;
  pthread_cond_broadcast(&_condition);
  pthread_mutex_unlock(&_mutex);
}

QueueStatus WorkQueue::Status()
{
  pthread_mutex_lock(&_mutex);
  QueueStatus status{_name, _priority, 0, _runs, _maxRunUs};
  for (WorkItem *item = _items; item != nullptr; item = item->_next) { ++status.items; }
  pthread_mutex_unlock(&_mutex);
  return status;
}

void WorkQueue::WaitUntil(uint64_t monotonicDeadline)
{
  const uint64_t now = os::Clock::nowMicroseconds();
  if (monotonicDeadline <= now) { return; }
  struct timespec deadline{};
  if (_conditionClock == CLOCK_MONOTONIC)
    {
      deadline.tv_sec = monotonicDeadline / 1000000ULL;
      deadline.tv_nsec = (monotonicDeadline % 1000000ULL) * 1000ULL;
    }
  else
    {
      clock_gettime(CLOCK_REALTIME, &deadline);
      const uint64_t delta = monotonicDeadline - now;
      deadline.tv_sec += delta / 1000000ULL;
      deadline.tv_nsec += (delta % 1000000ULL) * 1000ULL;
      deadline.tv_sec += deadline.tv_nsec / 1000000000L;
      deadline.tv_nsec %= 1000000000L;
    }
  (void)pthread_cond_timedwait(&_condition, &_mutex, &deadline);
}

void *WorkQueue::Entry(void *argument)
{
  auto *queue = static_cast<WorkQueue *>(argument);
  queue->Loop();
  return nullptr;
}

int WorkQueue::TaskEntry(int argc, char *argv[])
{
  if (argc < 2) { return -1; }
  const uintptr_t address = static_cast<uintptr_t>(strtoull(argv[1], nullptr, 16));
  if (address == 0) { return -1; }
  reinterpret_cast<WorkQueue *>(address)->Loop();
  return 0;
}

void WorkQueue::Loop()
{
  pthread_mutex_lock(&_mutex);
  while (!_exit)
    {
      const uint64_t now = os::Clock::nowMicroseconds();
      WorkItem *selected = nullptr;
      uint64_t selectedDeadline = UINT64_MAX;
      uint64_t nextWake = UINT64_MAX;
      for (WorkItem *item = _items; item != nullptr; item = item->_next)
        {
          if (!item->_enabled) { continue; }
          uint64_t due = item->_pending ? item->_deadline : UINT64_MAX;
          if (item->_period != 0 && item->_nextPeriodic < due)
            { due = item->_nextPeriodic; }
          if (due == UINT64_MAX) { continue; }
          const uint64_t earliest = item->_lastStart + item->_minimumInterval;
          if (item->_lastStart != 0 && due < earliest) { due = earliest; }
          if (due < nextWake) { nextWake = due; }
          if (due <= now && due < selectedDeadline)
            { selected = item; selectedDeadline = due; }
        }
      if (selected == nullptr)
        {
          if (nextWake == UINT64_MAX) { pthread_cond_wait(&_condition, &_mutex); }
          else { WaitUntil(nextWake); }
          continue;
        }
      selected->_pending = false;
      selected->_lastStart = now;
      if (selected->_period != 0) { selected->_nextPeriodic = now + selected->_period; }
      selected->_running = true;
      pthread_mutex_unlock(&_mutex);
      selected->Run();
      const uint64_t elapsed = os::Clock::nowMicroseconds() - now;
      pthread_mutex_lock(&_mutex);
      selected->_running = false;
      ++_runs;
      if (elapsed > _maxRunUs) { _maxRunUs = elapsed; }
      pthread_cond_broadcast(&_condition);
    }
  _workerExited = true;
  pthread_cond_broadcast(&_condition);
  pthread_mutex_unlock(&_mutex);
}
}
