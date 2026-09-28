/** @file output_chain_test.cpp
 * @brief 验证逻辑执行器到物理槽位的映射、SA互锁和硬故障处理。
 */
#include "robot/modules/Command.hpp"
#include "robot/output/MixingOutput.hpp"
#include "robot/control/Allocation.hpp"
#include "robot/control/ActuatorEffectivenessDualMagneticWheel.hpp"
#include "robot/params/ParamManager.hpp"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int os::Task::spawn(const char *, int, size_t, Entry, int, char *[])
{ assert(false); return -1; }
int os::Task::terminate(int) { assert(false); return -1; }
int CommandRouter::dispatch(const char *, int, char *[])
{ assert(false); return -1; }

class RecordingOutput final : public OutputInterface
{
public:
  bool init() override { return true; }
  bool updateOutputs(bool stop, const float *values, uint8_t count,
                     uint64_t now) override
  {
    (void)now;
    ++calls;
    stopped = stop;
    if (stop) { ++stopCalls; return true; }
    outputCount = count;
    for (uint8_t index = 0; index < count; ++index)
      { outputs[index] = values[index]; }
    if (failNext) { failNext = false; return false; }
    return true;
  }
  unsigned calls{};
  unsigned stopCalls{};
  bool stopped{true};
  bool failNext{false};
  uint8_t outputCount{};
  float outputs[MixingOutput::MaxOutputs]{};
};

static void publishInput(uint64_t now, ManualControl &manual,
                         ManualControlSwitches &switches)
{
  manual.timestamp = manual.timestampSample = now;
  switches.timestamp = switches.timestampSample = now;
  assert(manualControlTopic().publish(manual));
  assert(manualControlSwitchesTopic().publish(switches));
}

static void publishOutputs(uint64_t now, ManualControl &manual,
                           Allocation &allocation)
{
  manual.timestamp = manual.timestampSample = now;
  ActuatorMotors motors{};
  ActuatorServos servos{};
  allocation.allocate(manual, motors, servos);
  assert(actuatorMotorsTopic().publish(motors));
  assert(actuatorServosTopic().publish(servos));
}

static bool tick(uint64_t &now, ManualControl &manual,
                 ManualControlSwitches &switches, Command &command,
                 Allocation &allocation, MixingOutput &mixing)
{
  now += 20000;
  publishInput(now, manual, switches);
  command.update(now);
  publishOutputs(now, manual, allocation);
  return mixing.update(now);
}

int main()
{
  ParamManager &params = ParamManager::instance();
  assert(params.set(params.find("CA_ARM_SW"), int32_t{1}));
  assert(params.set(params.find("OUT_FAIL_ACT"), int32_t{2}));
  Command::setOutputInhibited(false);
  Command command;
  ActuatorEffectivenessDualMagneticWheel model;
  Allocation allocation(model);
  RecordingOutput driver;
  static const char *const functions[] = {
    "CAN_M0_FUNC", "CAN_M1_FUNC", "CAN_S0_FUNC", "CAN_S1_FUNC"
  };
  MixingOutput mixing(nullptr, driver, functions, 4);

  uint64_t now = 1000000;
  ManualControl manual{};
  manual.valid = true;
  manual.throttle = 0.4f;
  manual.yaw = -0.5f;
  ManualControlSwitches switches{};
  switches.valid = true;
  switches.switchCount = 12;
  switches.positions[0] = ManualControlSwitches::POSITION_ON;

  assert(tick(now, manual, switches, command, allocation, mixing));
  assert(driver.stopped);
  switches.positions[0] = ManualControlSwitches::POSITION_OFF;
  assert(tick(now, manual, switches, command, allocation, mixing));
  switches.positions[0] = ManualControlSwitches::POSITION_ON;
  assert(tick(now, manual, switches, command, allocation, mixing));
  assert(!driver.stopped && driver.outputCount == 4);
  assert(fabsf(driver.outputs[0] - 0.4f) < 0.0001f);
  assert(fabsf(driver.outputs[1] - 0.4f) < 0.0001f);
  assert(fabsf(driver.outputs[2] + 0.5f) < 0.0001f);
  assert(driver.outputs[3] == 0.0f);

  assert(params.set(params.find("CAN_S1_FUNC"), int32_t{0}));
  assert(mixing.updateParams());
  assert(tick(now, manual, switches, command, allocation, mixing));
  assert(!mixing.isFunctionSet(3) && driver.outputs[3] == 0.0f);

  // 上层数据第一次超时先归零，持续500 ms后再物理失能。
  now += 600000;
  publishInput(now, manual, switches);
  command.update(now);
  assert(mixing.update(now));
  assert(!driver.stopped && driver.outputs[0] == 0.0f &&
         driver.outputs[2] == 0.0f);
  now += 600000;
  publishInput(now, manual, switches);
  command.update(now);
  assert(mixing.update(now));
  assert(driver.stopped);

  publishOutputs(now, manual, allocation);
  assert(mixing.update(now));
  assert(!driver.stopped && fabsf(driver.outputs[0] - 0.4f) < 0.0001f);

  // RobotControl发布安全目标时执行器消息新鲜，原始RC采样时间允许保持旧值。
  ActuatorMotors safeMotors{};
  ActuatorServos safeServos{};
  assert(actuatorMotorsTopic().copy(safeMotors));
  assert(actuatorServosTopic().copy(safeServos));
  safeMotors.timestamp = safeServos.timestamp = now;
  safeMotors.timestampSample = safeServos.timestampSample = now - 600000;
  safeMotors.control[0] = safeMotors.control[1] = 0.0f;
  assert(actuatorMotorsTopic().publish(safeMotors));
  assert(actuatorServosTopic().publish(safeServos));
  assert(mixing.update(now));
  assert(!driver.stopped && driver.outputs[0] == 0.0f);

  ActuatorMotors motors{};
  assert(actuatorMotorsTopic().copy(motors));
  motors.control[0] = NAN;
  assert(actuatorMotorsTopic().publish(motors));
  assert(mixing.update(now));
  assert(!driver.stopped && driver.outputs[0] == 0.0f);

  publishOutputs(now, manual, allocation);
  driver.failNext = true;
  assert(!mixing.update(now));
  assert(driver.stopped);

  Command::setOutputInhibited(true);
  publishInput(now, manual, switches);
  command.update(now);
  assert(mixing.update(now));
  assert(driver.stopped);
  puts("OutputFunction mapping and safety chain test passed");
  return 0;
}
