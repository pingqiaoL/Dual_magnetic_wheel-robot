#include "robot/work_queue/WorkItem.hpp"
#include "robot/work_queue/WorkQueue.hpp"
#include "robot/work_queue/WorkQueueManager.hpp"
#include <assert.h>

namespace robot_wq
{
WorkItem::WorkItem(WorkQueue &queue) : _queue(&queue)
{ (void)Attach(0); }
WorkItem::WorkItem(const char *name, const wq_config_t &config)
  : _queue(WorkQueueFindOrCreate(config)), _name(name)
{ if (_queue != nullptr) { (void)Attach(0); } }
WorkItem::~WorkItem()
{
  // StopAndWait() must complete before derived state is destroyed.
  assert(!_attached);
}
bool WorkItem::Attach(uint64_t minimumIntervalUs)
{ return _queue != nullptr && _queue->Attach(*this, minimumIntervalUs); }
void WorkItem::ActivateSchedule()
{ if (_queue != nullptr) { _queue->Activate(*this); } }
void WorkItem::SetMinimumInterval(uint64_t intervalUs)
{ if (_queue != nullptr) { _queue->SetMinimumInterval(*this, intervalUs); } }
void WorkItem::ScheduleNow()
{ if (_queue != nullptr) { _queue->Schedule(*this, 0); } }
bool WorkItem::StopAndWait(uint32_t timeoutMs)
{ return _queue == nullptr || _queue->Detach(*this, timeoutMs); }
}
