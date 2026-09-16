/**
 * @file Thread.hpp
 * @brief 声明 NuttX pthread 的轻量封装，并负责安全复制线程启动参数。
 */

#pragma once

#include <pthread.h>
#include <stddef.h>

namespace os
{

/**
 * @class Thread
 * @brief 为上层模块提供线程创建、命名、参数传递和回收接口。
 */
class Thread
{
public:
  /** 模块线程入口类型，与 ModuleBase::run_trampoline 的参数形式一致。 */
  using Entry = int (*)(int argc, char *argv[]);

  /** 创建一个尚未启动的线程封装对象。 */
  Thread();

  /** 销毁封装对象；未回收的线程会被分离，防止 pthread 资源泄漏。 */
  ~Thread();

  /** 禁止复制 pthread 句柄。 */
  Thread(const Thread &) = delete;

  /** 禁止通过赋值共享 pthread 句柄。 */
  Thread &operator=(const Thread &) = delete;

  /** 创建线程，并深拷贝 argc/argv，避免 NSH 命令返回后参数失效。 */
  int start(const char *name, int priority, size_t stackSize,
            Entry entry, int argc, char *argv[]);

  /** 等待线程结束并回收 pthread 资源。 */
  int join();

  /** 判断当前 pthread 是否还需要 join。 */
  bool joinable() const;

  /** 返回创建线程时使用的调度优先级。 */
  int priority() const;

  /** 返回创建线程时配置的栈大小，单位为字节。 */
  size_t stackSize() const;

private:
  pthread_t _thread;
  bool _joinable;
  int _priority;
  size_t _stackSize;
};

} // namespace os
