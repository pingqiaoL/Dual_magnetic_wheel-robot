#include "robot/work_queue/WorkQueueManager.hpp"
#include <stdio.h>
#include <string.h>
namespace robot_wq
{
WorkQueue &SafetyQueue()
{
  const auto &config = wq_configurations::safety;
  static WorkQueue queue(config.name, config.priority, config.stacksize);
  return queue;
}
WorkQueue &ControlQueue()
{
  const auto &config = wq_configurations::control;
  static WorkQueue queue(config.name, config.priority, config.stacksize);
  return queue;
}
WorkQueue *WorkQueueFindOrCreate(const wq_config_t &config)
{
  const wq_config_t *known[] = {&wq_configurations::safety, &wq_configurations::control};
  for (unsigned i = 0; i < 2; ++i)
    {
      if (config.name != nullptr && strcmp(config.name, known[i]->name) == 0 &&
          config.priority == known[i]->priority && config.stacksize == known[i]->stacksize)
        {
          WorkQueue &queue = i == 0 ? SafetyQueue() : ControlQueue();
          return queue.Start() ? &queue : nullptr;
        }
    }
  return nullptr;
}
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
