/**
 * @file Thread.cpp
 * @brief 实现 pthread 创建与参数深拷贝，隔离上层模块和 NuttX 线程细节。
 */

#include "robot/os/Thread.hpp"

#include <sched.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace
{

/** 保存新线程真正使用的入口函数及命令行参数副本。 */
struct ThreadStartContext
{
  os::Thread::Entry entry;
  int argc;
  char **argv;
};

/** 释放线程启动参数及其字符串副本。 */
void freeStartContext(ThreadStartContext *context)
{
  if (context == nullptr)
    {
      return;
    }

  for (int index = 0; context->argv != nullptr && index < context->argc;
       ++index)
    {
      free(context->argv[index]);
    }

  free(context->argv);
  free(context);
}

/** 深拷贝 NSH 命令参数，使新线程不依赖命令线程的栈。 */
ThreadStartContext *copyStartContext(os::Thread::Entry entry,
                                     int argc, char *argv[])
{
  if (entry == nullptr || argc < 0)
    {
      return nullptr;
    }

  auto *context = static_cast<ThreadStartContext *>(
      calloc(1, sizeof(ThreadStartContext)));
  if (context == nullptr)
    {
      return nullptr;
    }

  context->entry = entry;
  context->argc = argc;

  if (argc == 0)
    {
      return context;
    }

  context->argv = static_cast<char **>(
      calloc(static_cast<size_t>(argc) + 1U, sizeof(char *)));
  if (context->argv == nullptr)
    {
      freeStartContext(context);
      return nullptr;
    }

  for (int index = 0; index < argc; ++index)
    {
      if (argv == nullptr || argv[index] == nullptr)
        {
          freeStartContext(context);
          return nullptr;
        }

      context->argv[index] = strdup(argv[index]);
      if (context->argv[index] == nullptr)
        {
          freeStartContext(context);
          return nullptr;
        }
    }

  return context;
}

/** pthread 固定入口：调用模块入口后释放命令参数副本。 */
void *pthreadEntry(void *argument)
{
  auto *context = static_cast<ThreadStartContext *>(argument);
  const int result = context->entry(context->argc, context->argv);
  freeStartContext(context);
  return reinterpret_cast<void *>(static_cast<intptr_t>(result));
}

} // namespace

namespace os
{

/** 初始化尚未绑定 pthread 的线程对象。 */
Thread::Thread() : _thread(0), _joinable(false), _priority(0), _stackSize(0)
{
}

/** 分离仍未回收的线程，正常模块退出应优先显式调用 join。 */
Thread::~Thread()
{
  if (_joinable)
    {
      pthread_detach(_thread);
    }
}

/** 配置线程属性，复制命令参数并创建 pthread。 */
int Thread::start(const char *name, int priority, size_t stackSize,
                  Entry entry, int argc, char *argv[])
{
  if (_joinable || entry == nullptr || argc < 0)
    {
      return -1;
    }

  ThreadStartContext *context = copyStartContext(entry, argc, argv);
  if (context == nullptr)
    {
      return -1;
    }

  pthread_attr_t attributes;
  int result = pthread_attr_init(&attributes);
  if (result != 0)
    {
      freeStartContext(context);
      return result;
    }

  struct sched_param scheduling;
  scheduling.sched_priority = priority;

  result = pthread_attr_setstacksize(&attributes, stackSize);
  if (result == 0)
    {
      result = pthread_attr_setinheritsched(&attributes,
                                            PTHREAD_EXPLICIT_SCHED);
    }

  if (result == 0)
    {
      result = pthread_attr_setschedparam(&attributes, &scheduling);
    }

  if (result == 0)
    {
      result = pthread_create(&_thread, &attributes, pthreadEntry, context);
    }

  pthread_attr_destroy(&attributes);

  if (result == 0)
    {
      _joinable = true;
      _priority = priority;
      _stackSize = stackSize;
      if (name != nullptr)
        {
          pthread_setname_np(_thread, name);
        }
    }
  else
    {
      freeStartContext(context);
    }

  return result;
}

/** 等待线程结束并清除本对象保存的 pthread 状态。 */
int Thread::join()
{
  if (!_joinable)
    {
      return 0;
    }

  const int result = pthread_join(_thread, nullptr);
  if (result == 0)
    {
      _joinable = false;
      _thread = 0;
    }

  return result;
}

/** 返回线程是否已经创建且尚未 join。 */
bool Thread::joinable() const
{
  return _joinable;
}

/** 返回线程的调度优先级。 */
int Thread::priority() const
{
  return _priority;
}

/** 返回线程栈大小。 */
size_t Thread::stackSize() const
{
  return _stackSize;
}

} // namespace os
