#include "robot/work_queue/WorkQueueManager.hpp"
#include <stdio.h>
namespace robot_wq
{
WorkQueue &SafetyQueue()
{
  static WorkQueue queue("wq_safety", 110, 8192);
  return queue;
}
WorkQueue &ControlQueue()
{
  static WorkQueue queue("wq_control", 105, 8192);
  return queue;
}
bool Initialize()
{ return SafetyQueue().Start() && ControlQueue().Start(); }
void PrintStatus()
{
  WorkQueue *queues[] = {&SafetyQueue(), &ControlQueue()};
  for (WorkQueue *queue : queues)
    {
      const QueueStatus status = queue->Status();
      printf("%s: priority %d, items %u, runs %llu, max Run %llu us\n",
             status.name, status.priority, status.items,
             static_cast<unsigned long long>(status.runs),
             static_cast<unsigned long long>(status.maxRunUs));
    }
}
}
