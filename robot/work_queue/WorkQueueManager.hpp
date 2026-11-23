#pragma once
#include "robot/work_queue/WorkQueue.hpp"
#include "robot/work_queue/WorkQueueConfigurations.hpp"
namespace robot_wq
{
/** RcUpdate + Command, preserving their existing priority 110. */
WorkQueue &SafetyQueue();
/** Failsafe + RobotControl + ControlAllocator, priority 105. */
WorkQueue &ControlQueue();
/** Find a configured queue and lazily start its worker. Unknown configs fail. */
WorkQueue *WorkQueueFindOrCreate(const wq_config_t &config);
void PrintStatus();
}
