/**
 * @file Task.cpp
 * @brief 使用 NuttX task API 实现长期运行模块的独立任务封装。
 */

#include "robot/os/Task.hpp"

#include <sched.h>//包含nuttx的线程头文件

namespace os
{

/** 创建独立 NuttX 任务；task_create 会把参数复制到新任务栈中。 */
int Task::spawn(const char *name, int priority, size_t stackSize,
                Entry entry, int argc, char *argv[])
{
  if (name == nullptr || entry == nullptr || argc < 0)
    {
      return -1;
    }

  return task_create(name, priority, static_cast<int>(stackSize),
                     entry, argv);
}

/** 强制删除未能响应退出请求的 NuttX 任务。 */
int Task::terminate(int taskId)
{
  return taskId >= 0 ? task_delete(taskId) : 0;
}

} // namespace os
