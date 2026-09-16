/**
 * @file rc_update_main.cpp
 * @brief 提供 NuttX NSH 的 rc_update 命令入口。
 */

#include "robot/modules/RcUpdate.hpp"

/** 将命令参数交给 RcUpdate 的 CRTP ModuleBase 入口。 */
extern "C" int main(int argc, char *argv[])
{
  return RcUpdate::main(argc, argv);
}
