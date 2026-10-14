#pragma once
#include "robot/work_queue/WorkItem.hpp"

namespace robot_wq
{
class ScheduledWorkItem : public WorkItem
{
public:
  explicit ScheduledWorkItem(WorkQueue &queue) : WorkItem(queue) {}
  bool InitSchedule(uint64_t minimumIntervalUs);
  void ScheduleDelayed(uint64_t delayUs);
  void ScheduleOnInterval(uint64_t intervalUs);
  /** Cancel pending/timed work, keeping the item registered for rescheduling. */
  void ScheduleClear();
};
}
