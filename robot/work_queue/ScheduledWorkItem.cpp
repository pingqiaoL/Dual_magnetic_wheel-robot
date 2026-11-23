#include "robot/work_queue/ScheduledWorkItem.hpp"
#include "robot/work_queue/WorkQueue.hpp"
namespace robot_wq
{
void ScheduledWorkItem::ScheduleDelayed(uint64_t delayUs)
{ if (_queue != nullptr) { _queue->Schedule(*this, delayUs); } }
void ScheduledWorkItem::ScheduleOnInterval(uint64_t intervalUs)
{ if (_queue != nullptr) { _queue->SetInterval(*this, intervalUs); } }
void ScheduledWorkItem::ScheduleClear()
{ if (_queue != nullptr) { _queue->Clear(*this); } }
}
