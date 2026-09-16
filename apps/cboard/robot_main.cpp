/**
 * @file robot_main.cpp
 * @brief 提供 NuttX NSH 的 robot 命令入口，并交给 RobotRuntime 统一解析。
 */

#include "robot/modules/RobotRuntime.hpp"

/** 将 NSH 传入的参数直接转交给 CRTP 模块父类处理。 */
extern "C" int main(int argc, char *argv[])
{
  return RobotRuntime::main(argc, argv);
}
