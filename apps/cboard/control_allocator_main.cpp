/**
 * @file control_allocator_main.cpp
 * @brief 提供 NuttX NSH 的 control_allocator 命令入口。
 */

#include "robot/modules/ControlAllocator.hpp"

/** 将 NSH 参数交给 CRTP ModuleBase 统一入口。 */
extern "C" int main(int argc, char *argv[])
{
  return ControlAllocator::main(argc, argv);
}
