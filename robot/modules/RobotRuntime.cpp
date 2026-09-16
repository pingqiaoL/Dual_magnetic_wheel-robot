/**
 * @file RobotRuntime.cpp
 * @brief 实现 RobotRuntime 的线程创建、对象创建、状态输出和周期循环。
 */

#include "robot/modules/RobotRuntime.hpp"

#include "robot/os/Clock.hpp"
#include "robot/params/ParamSystem.hpp"

#include <stdio.h>
#include <string.h>

/** 构造基础运行模块并清零运行计数。 */
RobotRuntime::RobotRuntime()
    : _loopCount(0),
      _initializationComplete(false),
      _lastInitializationNotice(0)
{
}

/** 创建独立 pthread，并由 ModuleBase::run_trampoline 接管线程入口。 */
int RobotRuntime::task_spawn(int argc, char *argv[])
{
  const int taskId = os::Task::spawn("robot_core", 100, 8192,
                                     &run_trampoline, argc, argv);
  if (taskId < 0)
    {
      __atomic_store_n(&_taskId, -1, __ATOMIC_RELEASE);
      return -1;
    }

  __atomic_store_n(&_taskId, taskId, __ATOMIC_RELEASE);
  return wait_until_running();
}

/** 在线程上下文中创建模块对象，当前版本不需要启动参数。 */
RobotRuntime *RobotRuntime::instantiate(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  if (!paramSystemInitialize())
    {
      fprintf(stderr, "parameter system initialization failed\n");
      return nullptr;
    }
  return new RobotRuntime();
}

/** 处理rcS发出的ready命令，公布全部模块已经初始化完成。 */
int RobotRuntime::custom_command(int argc, char *argv[])
{
  if (argc >= 1 && strcmp(argv[0], "ready") == 0)
    {
      RobotRuntime *instance = get_instance();
      if (instance == nullptr)
        {
          return print_usage("robot runtime is not running");
        }
      instance->markInitializationComplete();
      return 0;
    }
  return print_usage("unknown command");
}

/** 打印命令说明，供 NSH 帮助和错误提示使用。 */
int RobotRuntime::print_usage(const char *reason)
{
  if (reason != nullptr)
    {
      fprintf(stderr, "%s\n", reason);
    }

  printf("usage: robot {start|stop|status|ready}\n");
  return reason == nullptr ? 0 : -1;
}

/** 打印当前运行状态和循环计数。 */
int RobotRuntime::print_status()
{
  printf("running\n");
  printf("loops: %lu\n",
         static_cast<unsigned long>(
             __atomic_load_n(&_loopCount, __ATOMIC_ACQUIRE)));
  printf("initialization: %s\n",
         __atomic_load_n(&_initializationComplete, __ATOMIC_ACQUIRE)
             ? "complete"
             : "in_progress");
  printf("parameters: generation %lu, storage %lu, dirty %s\n",
         static_cast<unsigned long>(params().generation()),
         static_cast<unsigned long>(params().storageSequence()),
         params().dirty() ? "yes" : "no");
  return 0;
}

/** 每 100 ms 执行一次基础循环，收到 stop 请求后安全退出。 */
void RobotRuntime::run()
{
  while (!should_exit())
    {
      const uint64_t now = os::Clock::nowMicroseconds();
      if (!__atomic_load_n(&_initializationComplete, __ATOMIC_ACQUIRE) &&
          (_lastInitializationNotice == 0 ||
           now - _lastInitializationNotice >= InitializationNoticeIntervalUs))
        {
          printf("INITIALIZING [robot] modules are starting; keep SA OFF\n");
          _lastInitializationNotice = now;
        }
      __atomic_add_fetch(&_loopCount, 1U, __ATOMIC_RELAXED);
      params().pollAutoSave(now);
      os::Clock::sleepMilliseconds(100);
    }
}

/** 停止周期初始化提示，并向NSH公布系统可以接受SA操作。 */
void RobotRuntime::markInitializationComplete()
{
  const bool wasComplete = __atomic_exchange_n(
      &_initializationComplete, true, __ATOMIC_ACQ_REL);
  if (!wasComplete)
    {
      printf("CBoard initialization complete; set SA OFF, then ON to arm\n");
    }
}
