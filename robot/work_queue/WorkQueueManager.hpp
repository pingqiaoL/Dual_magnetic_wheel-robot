#pragma once
#include "robot/work_queue/WorkQueue.hpp"
namespace robot_wq
{
/** RcUpdate + Command, preserving their existing priority 110. */
WorkQueue &SafetyQueue();
/** Failsafe + RobotControl + ControlAllocator, priority 105. */
WorkQueue &ControlQueue();
bool Initialize();
void PrintStatus();
}
