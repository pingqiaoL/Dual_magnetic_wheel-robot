/**
 * @file ModuleBase.hpp
 * @brief 提供仿 PX4 CRTP 写法的模块公共入口和生命周期管理。
 *
 * ModuleBase<T> 在对象创建前即可通过子类 T 的静态函数处理命令。
 * 子类需要实现 task_spawn、instantiate、custom_command 和 print_usage，
 * 并重写 run；父类统一完成 start、stop、status、线程跳板和对象释放。
 * 模板的全部实现必须放在头文件中，因此本模块没有对应的 .cpp 文件。
 */

#pragma once

#include "robot/os/Clock.hpp"
#include "robot/os/Task.hpp"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

/**
 * @class ModuleBase
 * @brief 使用 CRTP 为不同子类分别保存模块实例和任务状态。
 *
 * @tparam T 继承 ModuleBase<T> 的具体模块类型。
 */
template<class T>
class ModuleBase
{
public:
  /** 初始化模块对象的退出标志。 */
  ModuleBase() : _taskShouldExit(false)
  {
  }

  /** 允许通过父类指针正确析构具体模块对象。 */
  virtual ~ModuleBase() = default;

  /** 禁止复制模块对象，避免一个任务对应多个生命周期状态。 */
  ModuleBase(const ModuleBase &) = delete;

  /** 禁止给模块对象赋值，避免复制退出标志。 */
  ModuleBase &operator=(const ModuleBase &) = delete;

  /**
   * @brief 模块命令统一入口，解析 start、stop、status 和帮助命令。
   * @return 成功返回 0，失败返回 -1。
   */
  static int main(int argc, char *argv[])
  {
    if (argc <= 1 || strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "help") == 0 ||
        strcmp(argv[1], "info") == 0 ||
        strcmp(argv[1], "usage") == 0)
      {
        return T::print_usage();
      }

    if (strcmp(argv[1], "start") == 0)
      {
        return start_command_base(argc - 1, argv + 1);
      }

    if (strcmp(argv[1], "stop") == 0)
      {
        return stop_command();
      }

    if (strcmp(argv[1], "status") == 0)
      {
        return status_command();
      }

    lock_module();
    const int result = T::custom_command(argc - 1, argv + 1);
    unlock_module();
    return result;
  }

  /**
   * @brief 新线程入口，在任务线程内创建子类对象并执行 run。
   * @return 模块正常结束返回 0，创建对象失败返回 -1。
   */
  static int run_trampoline(int argc, char *argv[])
  {
    if (argc > 0)
      {
        --argc;
        ++argv;
      }

    T *object = T::instantiate(argc, argv);
    __atomic_store_n(&_object, object, __ATOMIC_RELEASE);

    int result = 0;
    if (object != nullptr)
      {
        object->run();
      }
    else
      {
        fprintf(stderr, "module object creation failed\n");
        result = -1;
      }

    exit_and_cleanup();
    return result;
  }

  /** 判断该类型的模块任务当前是否处于启动或运行状态。 */
  static bool is_running()
  {
    return __atomic_load_n(&_taskId, __ATOMIC_ACQUIRE) != -1;
  }

  /** 打印模块的默认运行状态，子类可重写以补充业务数据。 */
  virtual int print_status()
  {
    printf("running\n");
    return 0;
  }

  /** 模块任务主循环，由具体子类重写。 */
  virtual void run() = 0;

protected:
  /** 向任务线程发出协作式停止请求。 */
  virtual void request_stop()
  {
    __atomic_store_n(&_taskShouldExit, true, __ATOMIC_RELEASE);
  }

  /** 供子类 run 循环判断是否需要退出。 */
  bool should_exit() const
  {
    return __atomic_load_n(&_taskShouldExit, __ATOMIC_ACQUIRE);
  }

  /** 返回正在运行的子类对象，模块未运行时返回 nullptr。 */
  static T *get_instance()
  {
    return __atomic_load_n(&_object, __ATOMIC_ACQUIRE);
  }

  /**
   * @brief 在线程内删除模块对象并公布任务已经停止。
   *
   * 此函数只由 run_trampoline 在 run 返回后调用。
   */
  static void exit_and_cleanup()
  {
    lock_module();
    T *object = __atomic_load_n(&_object, __ATOMIC_ACQUIRE);
    delete object;
    __atomic_store_n(&_object, static_cast<T *>(nullptr), __ATOMIC_RELEASE);
    __atomic_store_n(&_taskId, -1, __ATOMIC_RELEASE);
    unlock_module();
  }

  /** 等待新线程完成对象创建，使 start 返回时模块已经可以使用。 */
  static int wait_until_running(int timeoutMilliseconds = 1000)
  {
    int elapsedMilliseconds = 0;
    while (get_instance() == nullptr && is_running() &&
           elapsedMilliseconds < timeoutMilliseconds)
      {
        os::Clock::sleepMilliseconds(2);
        elapsedMilliseconds += 2;
      }

    if (get_instance() == nullptr)
      {
        fprintf(stderr, "module start timed out\n");
        return -1;
      }

    return 0;
  }

  /** 每种子类独立拥有的当前运行对象指针。 */
  static T *_object;

  /** 每种子类独立拥有的任务标志，-1 表示未运行，1 表示启动或运行中。 */
  static int _taskId;

private:
  /** 检查重复启动并调用子类 task_spawn 创建任务。 */
  static int start_command_base(int argc, char *argv[])
  {
    lock_module();

    if (is_running() || _stopInProgress)
      {
        fprintf(stderr, "module is running or stopping\n");
        unlock_module();
        return -1;
      }

    const int result = T::task_spawn(argc, argv);
    if (result != 0)
      {
        fprintf(stderr, "module task start failed: %d\n", result);
      }

    unlock_module();
    return result;
  }

  /** 请求模块退出，最多等待五秒并回收线程资源。 */
  static int stop_command()
  {
    lock_module();

    if (_stopInProgress)
      {
        fprintf(stderr, "module stop already in progress\n");
        unlock_module();
        return -1;
      }

    if (!is_running())
      {
        unlock_module();
        printf("not running\n");
        return 0;
      }

    _stopInProgress = true;

    T *object = get_instance();
    if (object != nullptr)
      {
        object->request_stop();
      }

    unlock_module();

    int elapsedMilliseconds = 0;
    while (is_running() && elapsedMilliseconds < 5000)
      {
        lock_module();
        object = get_instance();
        if (object != nullptr)
          {
            object->request_stop();
          }
        unlock_module();

        os::Clock::sleepMilliseconds(10);
        elapsedMilliseconds += 10;
      }

    if (is_running())
      {
        lock_module();
        _stopInProgress = false;
        unlock_module();
      lock_module();
      const int taskId = __atomic_load_n(&_taskId, __ATOMIC_ACQUIRE);
      (void)os::Task::terminate(taskId);
      T *staleObject = __atomic_load_n(&_object, __ATOMIC_ACQUIRE);
      delete staleObject;
      __atomic_store_n(&_object, static_cast<T *>(nullptr), __ATOMIC_RELEASE);
      __atomic_store_n(&_taskId, -1, __ATOMIC_RELEASE);
      _stopInProgress = false;
      unlock_module();
      fprintf(stderr, "module stop timed out; task terminated\n");
      return -1;
    }

    lock_module();
    _stopInProgress = false;
    unlock_module();

    return 0;
  }

  /** 在模块运行时调用子类 print_status 输出状态。 */
  static int status_command()
  {
    lock_module();
    T *object = get_instance();

    if (!is_running() || object == nullptr)
      {
        printf("not running\n");
        unlock_module();
        return -1;
      }

    const int result = object->print_status();
    unlock_module();
    return result;
  }

  /** 获取该模板子类专用的生命周期互斥锁。 */
  static pthread_mutex_t &module_mutex()
  {
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    return mutex;
  }

  /** 加锁，防止命令线程与模块退出过程同时修改对象。 */
  static void lock_module()
  {
    pthread_mutex_lock(&module_mutex());
  }

  /** 解锁，允许其他命令或模块退出过程访问对象。 */
  static void unlock_module()
  {
    pthread_mutex_unlock(&module_mutex());
  }

  /** 标记 stop 命令正在等待线程退出，阻止并发 start 或第二个 stop。 */
  static bool _stopInProgress;

  /** 当前模块对象的协作式退出标志。 */
  bool _taskShouldExit;
};

/** 为每一种 ModuleBase<T> 子类定义独立的对象指针。 */
template<class T>
T *ModuleBase<T>::_object = nullptr;

/** 为每一种 ModuleBase<T> 子类定义独立的任务状态。 */
template<class T>
int ModuleBase<T>::_taskId = -1;

/** 为每一种 ModuleBase<T> 子类定义独立的停止命令状态。 */
template<class T>
bool ModuleBase<T>::_stopInProgress = false;
