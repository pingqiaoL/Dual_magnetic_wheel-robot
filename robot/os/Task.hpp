/**
 * @file Task.hpp
 * @brief 声明独立 NuttX 任务创建接口，避免模块依附于临时 NSH 命令线程。
 */

#pragma once

#include <stddef.h>

namespace os
{

/**
 * @class Task
 * @brief 封装 NuttX task_create/task_delete，供长期运行的模块使用。
 */
class Task
{
public:
  /** 独立任务入口类型，与 ModuleBase::run_trampoline 一致。 */
  using Entry = int (*)(int argc, char *argv[]);

  /**
   * @brief 创建具有独立任务组和生命周期的 NuttX 任务。
   * @return 成功返回正数任务 ID，失败返回 -1。
   */
  static int spawn(const char *name, int priority, size_t stackSize,
                   Entry entry, int argc, char *argv[]);

  /** 强制结束指定任务，仅用于协作式停止超时后的故障恢复。 */
  static int terminate(int taskId);
};

} // namespace os
