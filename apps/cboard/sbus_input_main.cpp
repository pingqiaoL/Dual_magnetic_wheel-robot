/**
 * @file sbus_input_main.cpp
 * @brief 提供 NuttX NSH 的 sbus_input 命令入口。
 */

#include "robot/modules/SbusInput.hpp"

/** 将命令参数交给 SbusInput 的 CRTP ModuleBase 入口。 */
extern "C" int main(int argc, char *argv[])
{
  return SbusInput::main(argc, argv);
}
