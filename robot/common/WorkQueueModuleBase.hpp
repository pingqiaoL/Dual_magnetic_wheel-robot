#pragma once
#include "robot/common/ModuleBase.hpp"
#include "robot/work_queue/ScheduledWorkItem.hpp"
#include "robot/work_queue/WorkQueueManager.hpp"

/** Reuses NSH lifecycle, with a shared worker rather than a per-module task. */
template<class T>
class WorkQueueModuleBase : public ModuleBase<T>, public robot_wq::ScheduledWorkItem
{
public:
  WorkQueueModuleBase(robot_wq::WorkQueue &queue, uint64_t periodUs)
    : ScheduledWorkItem(queue), _periodUs(periodUs) {}
  static int task_spawn(int argc, char *argv[])
  {
    T *object = T::instantiate(argc, argv);
    if (object == nullptr) { return -1; }
    WorkQueueModuleBase *base = object;
    if (!object->InitSchedule(base->_periodUs)) { delete object; return -1; }
    if (!base->initWorkQueue())
      {
        base->unregisterCallbacks();
        (void)object->StopAndWait();
        delete object;
        return -1;
      }
    __atomic_store_n(&ModuleBase<T>::_object, object, __ATOMIC_RELEASE);
    __atomic_store_n(&ModuleBase<T>::_taskId,
                     ModuleBase<T>::WorkQueueTaskId, __ATOMIC_RELEASE);
    object->ScheduleOnInterval(base->_periodUs);
    object->ScheduleNow();
    return 0;
  }

protected:
  virtual bool initWorkQueue() = 0;
  virtual void unregisterCallbacks() = 0;
  virtual void onWorkQueueStopped() = 0;

private:
  // Never called: work queue modules execute Run(), not run_trampoline().
  void run() final {}
protected:
  bool stop_work_queue() final
  {
    unregisterCallbacks();
    if (!StopAndWait()) { return false; }
    onWorkQueueStopped();
    return true;
  }
  const uint64_t _periodUs;
};
