/**
 * @file RobotRuntime.hpp
 * @brief 声明机器人基础运行模块，并展示 ModuleBase 的标准继承写法。
 */

#pragma once

#include "robot/common/ModuleBase.hpp"

#include <stdint.h>

/**
 * @class RobotRuntime
 * @brief 第一版下位机的基础任务，当前只周期计数，不访问任何执行器。
 */
class RobotRuntime final : public ModuleBase<RobotRuntime>
{
public:
  /** 构造基础运行模块并清零运行计数。 */
  RobotRuntime();

  /** 创建 robot_core 线程，并把父类跳板函数作为线程入口。 */
  static int task_spawn(int argc, char *argv[]);

  /** 在线程中解析参数并创建 RobotRuntime 对象。 */
  static RobotRuntime *instantiate(int argc, char *argv[]);

  /** 返回统一命令路由使用的固定模块名称。 */
  static const char *command_name() { return "robot"; }

  /** 打印 robot 命令的用法和可用子命令。 */
  static int print_usage(const char *reason = nullptr);

  /** 打印模块运行状态和已经执行的循环次数。 */
  int print_status() override;

  /** 执行基础模块的周期循环，收到停止请求后返回。 */
  void run() override;

private:
  /** 唯一扩展命令路由可以访问受保护的模块实例。 */
  friend class CommandRouter;
  static constexpr uint64_t InitializationNoticeIntervalUs = 2000000ULL;

  /** 标记rcS中的所有模块启动命令已经执行完成。 */
  void markInitializationComplete();

  /** 记录任务启动后已经完成的 100 ms 循环次数。 */
  uint32_t _loopCount;

  /** rcS尚未调用 robot ready 时保持为false。 */
  bool _initializationComplete;

  /** 记录上一次初始化进度提示时间。 */
  uint64_t _lastInitializationNotice;
};
