/**
 * @file can_output_main.cpp
 * @brief 提供 NuttX NSH 的 can_output 命令入口。
 */

#include "robot/output/CanOutput.hpp"

/** 将 NSH 参数交给 CRTP ModuleBase 统一入口。 */
extern "C" int main(int argc, char *argv[])
{
  return CanOutput::main(argc, argv);
}
