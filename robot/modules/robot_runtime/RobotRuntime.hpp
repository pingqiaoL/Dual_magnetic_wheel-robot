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

  /** 打印 robot 命令的用法和可用子命令。 */
  static int print_usage(const char *reason = nullptr);

  /** 处理手动robot ready诊断，并立即核验启动链中的所有必需模块。 */
  static int custom_command(int argc, char *argv[]);

  /** 打印模块运行状态和已经执行的循环次数。 */
  int print_status() override;

  /** 执行基础模块的周期循环，收到停止请求后返回。 */
  void run() override;

private:
  static constexpr uint64_t InitializationNoticeIntervalUs = 2000000ULL;
  static constexpr uint64_t InitializationFailureTimeoutUs = 5000000ULL;

  /** 检查启动链中的必需模块；report为true时逐项打印缺失模块。 */
  static bool requiredModulesRunning(bool report);

  /** 标记全部必需模块均已进入运行状态。 */
  void markInitializationComplete();

  /** 标记启动核验发现模块缺失，使周期提示不再误报为仍在启动。 */
  void markInitializationFailed();

  /** 记录任务启动后已经完成的 100 ms 循环次数。 */
  uint32_t _loopCount;

  /** 全部必需模块尚未进入运行状态时保持为false。 */
  bool _initializationComplete;

  /** 自动或手动核验超时后置位，提示用户查看缺失模块。 */
  bool _initializationFailed;

  /** 记录上一次初始化进度提示时间。 */
  uint64_t _lastInitializationNotice;

  /** 记录第一次运行初始化检查的时间，用于判断启动超时。 */
  uint64_t _initializationStartedAt;
};
