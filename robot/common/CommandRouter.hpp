/** @file CommandRouter.hpp
 * @brief 集中分发所有模块的扩展命令，生命周期仍由ModuleBase管理。
 */
#pragma once

/** 扩展命令的唯一分发器；调用者必须持有目标模块的生命周期锁。 */
class CommandRouter
{
public:
  /** 按模块名称执行扩展命令；argc/argv从子命令开始。 */
  static int dispatch(const char *module, int argc, char *argv[]);

private:
  /** 处理CAN协议、映射、设零、清错及安全禁止命令。 */
  static int handleCan(int argc, char *argv[]);
  /** 处理运行时初始化完成通知。 */
  static int handleRobot(int argc, char *argv[]);
  /** 处理command自身命令，并转发对其他模块的完整命令。 */
  static int handleCommand(int argc, char *argv[]);
};
