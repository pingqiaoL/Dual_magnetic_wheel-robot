/**
 * @file RobotRuntime.cpp
 * @brief 实现 RobotRuntime 的线程创建、对象创建、状态输出和周期循环。
 */

// NuttX Make对命令源文件定义main宏；显式命名入口无需重命名，
// 取消该宏以避免改写ModuleBase::main等C++成员名称。
#ifdef main
#undef main
#endif

#include "robot/modules/RobotRuntime.hpp"

#include "robot/modules/Command.hpp"
#include "robot/modules/ControlAllocator.hpp"
#include "robot/modules/Failsafe.hpp"
#include "robot/modules/RcUpdate.hpp"
#include "robot/modules/RobotControl.hpp"
#include "robot/modules/SbusInput.hpp"
#include "robot/os/Clock.hpp"
#include "robot/output/CanOutput.hpp"
#include "robot/params/ParamSystem.hpp"

#include <stdio.h>
#include <string.h>

/** 构造基础运行模块并清零运行计数。 */
RobotRuntime::RobotRuntime()
    : _loopCount(0),
      _initializationComplete(false),
      _initializationFailed(false),
      _lastInitializationNotice(0),
      _initializationStartedAt(0)
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

/** 检查启动链中的每个必需模块，可选择是否输出失败点。 */
bool RobotRuntime::requiredModulesRunning(bool report)
{
  bool ready = true;
  if (!RcUpdate::is_running())
    {
      if (report) { fprintf(stderr, "INITIALIZATION ERROR: rc_update is not running\n"); }
      ready = false;
    }
  if (!SbusInput::is_running())
    {
      if (report) { fprintf(stderr, "INITIALIZATION ERROR: sbus_input is not running\n"); }
      ready = false;
    }
  if (!Failsafe::is_running())
    {
      if (report) { fprintf(stderr, "INITIALIZATION ERROR: failsafe is not running\n"); }
      ready = false;
    }
  if (!RobotControl::is_running())
    {
      if (report) { fprintf(stderr, "INITIALIZATION ERROR: robot_control is not running\n"); }
      ready = false;
    }
  if (!ControlAllocator::is_running())
    {
      if (report) { fprintf(stderr, "INITIALIZATION ERROR: control_allocator is not running\n"); }
      ready = false;
    }
  if (!CanOutput::is_running())
    {
      if (report) { fprintf(stderr, "INITIALIZATION ERROR: can_output is not running\n"); }
      ready = false;
    }
  if (!Command::is_running())
    {
      if (report) { fprintf(stderr, "INITIALIZATION ERROR: command is not running\n"); }
      ready = false;
    }
  return ready;
}

/** 提供手动ready诊断；正常启动不依赖脚本调用该命令。 */
int RobotRuntime::custom_command(int argc, char *argv[])
{
  if (argc != 1 || strcmp(argv[0], "ready") != 0)
    { return print_usage("unknown command"); }

  RobotRuntime *instance = get_instance();
  if (instance == nullptr)
    { return print_usage("robot runtime is not running"); }
  if (!requiredModulesRunning(true))
    {
      instance->markInitializationFailed();
      fprintf(stderr,
              "INITIALIZATION ERROR: start missing modules, then run 'robot ready'\n");
      return -1;
    }
  instance->markInitializationComplete();
  return 0;
}

/** 打印当前运行状态和循环计数。 */
int RobotRuntime::print_status()
{
  printf("running\n");
  printf("loops: %lu\n",
         static_cast<unsigned long>(
             __atomic_load_n(&_loopCount, __ATOMIC_ACQUIRE)));
  const bool complete = __atomic_load_n(&_initializationComplete,
                                         __ATOMIC_ACQUIRE);
  const bool failed = __atomic_load_n(&_initializationFailed,
                                       __ATOMIC_ACQUIRE);
  printf("initialization: %s\n",
         complete ? "complete" : (failed ? "failed" : "in_progress"));
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
      if (_initializationStartedAt == 0) { _initializationStartedAt = now; }
      const bool complete = __atomic_load_n(&_initializationComplete,
                                             __ATOMIC_ACQUIRE);
      if (!complete && requiredModulesRunning(false))
        {
          markInitializationComplete();
        }
      else if (!complete &&
               !__atomic_load_n(&_initializationFailed, __ATOMIC_ACQUIRE) &&
               now - _initializationStartedAt >= InitializationFailureTimeoutUs)
        {
          (void)requiredModulesRunning(true);
          markInitializationFailed();
        }

      if (!__atomic_load_n(&_initializationComplete, __ATOMIC_ACQUIRE) &&
          (_lastInitializationNotice == 0 ||
           now - _lastInitializationNotice >= InitializationNoticeIntervalUs))
        {
          if (__atomic_load_n(&_initializationFailed, __ATOMIC_ACQUIRE))
            {
              printf("INITIALIZATION FAILED [robot] run 'robot ready' to list missing modules\n");
            }
          else
            {
              printf("INITIALIZING [robot] modules are starting; keep SA OFF\n");
            }
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
  __atomic_store_n(&_initializationFailed, false, __ATOMIC_RELEASE);
  const bool wasComplete = __atomic_exchange_n(
      &_initializationComplete, true, __ATOMIC_ACQ_REL);
  if (!wasComplete)
    {
      printf("CBoard initialization complete; set SA OFF, then ON to arm\n");
    }
}

/** 记录启动核验失败；缺失模块恢复后周期检查仍可自动转为完成。 */
void RobotRuntime::markInitializationFailed()
{
  __atomic_store_n(&_initializationComplete, false, __ATOMIC_RELEASE);
  __atomic_store_n(&_initializationFailed, true, __ATOMIC_RELEASE);
}

/** 提供NSH的robot命令入口，转交ModuleBase统一处理生命周期。 */
extern "C" int robot_main(int argc, char *argv[])
{
  return RobotRuntime::main(argc, argv);
}
