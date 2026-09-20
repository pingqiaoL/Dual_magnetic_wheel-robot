/** @file CommandRouter.cpp
 * @brief 集中实现扩展NSH命令，保留旧入口并支持command统一转发。
 */
#include "robot/common/CommandRouter.hpp"
#include "robot/modules/Command.hpp"
#include "robot/modules/RobotRuntime.hpp"
#include "robot/modules/ControlAllocator.hpp"
#include "robot/modules/SbusInput.hpp"
#include "robot/modules/RcUpdate.hpp"
#include "robot/output/CanOutput.hpp"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** 独立参数命令仍保留在参数模块，由统一入口转发完整参数命令。 */
extern "C" int param_main(int argc, char *argv[]);

/** 在ModuleBase已经持有目标生命周期锁时处理扩展命令。 */
int CommandRouter::dispatch(const char *module, int argc, char *argv[])
{
  if (strcmp(module, "command") == 0) { return handleCommand(argc, argv); }
  if (strcmp(module, "can_output") == 0) { return handleCan(argc, argv); }
  if (strcmp(module, "robot") == 0) { return handleRobot(argc, argv); }
  if (strcmp(module, "rc_update") == 0)
    { return RcUpdate::print_usage("unknown command"); }
  if (strcmp(module, "sbus_input") == 0)
    { return SbusInput::print_usage("unknown command"); }
  if (strcmp(module, "control_allocator") == 0)
    { return ControlAllocator::print_usage("unknown command"); }
  return Command::print_usage("unknown module");
}

/** ready仅由启动脚本在全部启动成功后发送。 */
int CommandRouter::handleRobot(int argc, char *argv[])
{
  if (argc == 1 && strcmp(argv[0], "ready") == 0)
    {
      RobotRuntime *instance = RobotRuntime::get_instance();
      if (instance == nullptr)
        { return RobotRuntime::print_usage("robot runtime is not running"); }
      instance->markInitializationComplete();
      return 0;
    }
  return RobotRuntime::print_usage("unknown command");
}

/** 手动使能只解除禁止，不绕过SA；完整模块命令仍经过其生命周期锁。 */
int CommandRouter::handleCommand(int argc, char *argv[])
{
  if (argc == 1 && (strcmp(argv[0], "enable") == 0 ||
                    strcmp(argv[0], "disable") == 0))
    {
      const bool inhibit = strcmp(argv[0], "disable") == 0;
      Command::setOutputInhibited(inhibit);
      printf(inhibit ? "output inhibit active\n"
                     : "output inhibit cleared; RC safety still required\n");
      return 0;
    }
  if (argc < 2) { return Command::print_usage("module and subcommand required"); }
  // 不转发command自身，避免重复获取当前模块的非递归生命周期锁。
  if (strcmp(argv[0], "can_output") == 0) { return CanOutput::main(argc, argv); }
  if (strcmp(argv[0], "control_allocator") == 0)
    { return ControlAllocator::main(argc, argv); }
  if (strcmp(argv[0], "rc_update") == 0) { return RcUpdate::main(argc, argv); }
  if (strcmp(argv[0], "sbus_input") == 0) { return SbusInput::main(argc, argv); }
  if (strcmp(argv[0], "robot") == 0) { return RobotRuntime::main(argc, argv); }
  if (strcmp(argv[0], "param") == 0) { return param_main(argc, argv); }
  return Command::print_usage("unsupported module");
}

/** 唯一位置处理CAN扩展命令，旧can_output入口仍可直接调用。 */
int CommandRouter::handleCan(int argc, char *argv[])
{
  if (argc == 1 && strcmp(argv[0], "protocols") == 0)
    { printf("0  disabled\n1  damiao\n"); return 0; }
  if (argc == 1 && (strcmp(argv[0], "enable") == 0 ||
                    strcmp(argv[0], "disable") == 0))
    { return handleCommand(argc, argv); }

  CanOutput *instance = CanOutput::get_instance();
  if (instance == nullptr) { return CanOutput::print_usage("can_output is not running"); }
  if (argc == 1 && strcmp(argv[0], "map") == 0)
    { instance->printMap(); return 0; }
  if (argc == 3 && (strcmp(argv[0], "zero") == 0 || strcmp(argv[0], "clear") == 0))
    {
      if (strcmp(argv[1], "motor") != 0 && strcmp(argv[1], "servo") != 0)
        { return CanOutput::print_usage("type must be motor or servo"); }
      const ActuatorType type = strcmp(argv[1], "motor") == 0
                                 ? ActuatorType::Motor : ActuatorType::Servo;
      errno = 0;
      char *end = nullptr;
      const long index = strtol(argv[2], &end, 0);
      const uint8_t count = type == ActuatorType::Motor
                              ? CanOutput::MotorCount : CanOutput::ServoCount;
      if (errno != 0 || end == argv[2] || *end != '\0' || index < 0 || index >= count)
        { return CanOutput::print_usage("actuator index is out of range"); }
      os::LockGuard guard(instance->_outputMutex);
      if (!guard.locked()) { return -1; }
      ActuatorArmed safety{};
      if (!Command::outputInhibited() ||
          !actuatorArmedTopic().copy(safety) || safety.armed || !safety.lockdown ||
          instance->_disablePending ||
          __atomic_load_n(&instance->_protocolArmed, __ATOMIC_ACQUIRE))
        { return CanOutput::print_usage("disable output and wait for disarm before zero/clear"); }
      return instance->sendSpecial(type, static_cast<uint8_t>(index),
                                    strcmp(argv[0], "zero") == 0 ? 0xfe : 0xfb)
               ? 0 : -1;
    }
  return CanOutput::print_usage("unknown command");
}
