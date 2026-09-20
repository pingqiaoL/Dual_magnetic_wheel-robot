/** @file output_chain_test.cpp
 * @brief 验证遥控安全状态、矩阵分配和虚函数输出回调的完整消息链路。
 */
#include "robot/modules/Command.hpp"
#include "robot/output/MixingOutput.hpp"
#include "robot/control/Allocation.hpp"
#include "robot/control/ActuatorEffectivenessDualMagneticWheel.hpp"
#include "robot/params/ParamManager.hpp"
#include <assert.h>
#include <math.h>
#include <stdio.h>

/** 本测试通过update验收消息链路，不允许意外调用真实任务创建。 */
int os::Task::spawn(const char *, int, size_t, Entry, int, char *[])
{
  assert(false && "host message test must not create a NuttX task");
  return -1;
}

/** 本测试不涉及任务强制删除，意外调用立即失败。 */
int os::Task::terminate(int)
{
  assert(false && "host message test must not terminate a NuttX task");
  return -1;
}

/** NSH命令路由在固件编译中验收，本测试不允许绕过消息调用命令。 */
int CommandRouter::dispatch(const char *, int, char *[])
{
  assert(false && "host message test must use topics instead of NSH commands");
  return -1;
}

/** 记录虚函数回调，用注入的失败代替真实硬件写入失败。 */
class RecordingOutput final : public OutputInterface
{
public:
  /** 无硬件测试接口初始化。 */
  bool init() override { return true; }
  /** 记录电机、舵机目标和停止请求。 */
  bool updateOutputs(ActuatorType type, bool stop, const float *values,
                     uint8_t count, uint64_t now) override
  {
    (void)now;
    const unsigned kind = type == ActuatorType::Motor ? 0 : 1;
    ++calls[kind];
    stopped[kind] = stop;
    if (stop) { ++stopCalls[kind]; return true; }
    counts[kind] = count;
    for (uint8_t i = 0; i < count; ++i) { outputs[kind][i] = values[i]; }
    if (failNext && type == ActuatorType::Motor) { failNext = false; return false; }
    return true;
  }
  unsigned calls[2]{};
  unsigned stopCalls[2]{};
  bool stopped[2]{true, true};
  uint8_t counts[2]{};
  float outputs[2][8]{};
  bool failNext{false};
};

/** 通过实际topic发布输入，驱动一次安全、分配和公共输出更新。 */
class OutputChainTest
{
public:
  /** 使用完整公开接口验证，不直接修改任何被测类的私有状态。 */
  static void run()
  {
    ParamManager &params = ParamManager::instance();
    assert(params.set(params.find("CA_ARM_SW"), int32_t{1}));
    Command::setOutputInhibited(false);
    Command command;
    ActuatorEffectivenessDualMagneticWheel model;
    Allocation allocation(model);
    RecordingOutput driver;
    MixingOutput mixing(driver, 2, 2);
    uint64_t now = 1000000;
    ManualControl manual{};
    manual.valid = true;
    manual.throttle = 0.4f;
    manual.yaw = -0.5f;
    ManualControlSwitches switches{};
    switches.valid = true;
    switches.switchCount = 12;
    switches.positions[0] = ManualControlSwitches::POSITION_ON;

    // 上电SA为ON不得使能，OFF之后ON才能使能两路动力与两路转向。
    tick(now, manual, switches, command, allocation, mixing);
    assert(driver.stopped[0] && driver.stopped[1]);
    switches.positions[0] = ManualControlSwitches::POSITION_OFF;
    tick(now, manual, switches, command, allocation, mixing);
    assert(driver.stopped[0] && driver.stopped[1]);
    switches.positions[0] = ManualControlSwitches::POSITION_ON;
    tick(now, manual, switches, command, allocation, mixing);
    assert(!driver.stopped[0] && !driver.stopped[1]);
    assert(driver.counts[0] == 2 && driver.counts[1] == 2);
    assert(fabsf(driver.outputs[0][0] - 0.4f) < 0.0001f);
    assert(fabsf(driver.outputs[1][0] + 0.5f) < 0.0001f);
    assert(driver.outputs[1][1] == 0.0f);

    // OFF失能；已经完成的上电检查不会在暂时失联后重新执行。
    switches.positions[0] = ManualControlSwitches::POSITION_OFF;
    tick(now, manual, switches, command, allocation, mixing);
    assert(driver.stopped[0] && driver.stopped[1]);
    switches.positions[0] = ManualControlSwitches::POSITION_ON;
    tick(now, manual, switches, command, allocation, mixing);
    manual.valid = false;
    tick(now, manual, switches, command, allocation, mixing);
    assert(driver.stopped[0] && driver.stopped[1]);
    manual.valid = true;
    tick(now, manual, switches, command, allocation, mixing);
    assert(!driver.stopped[0] && !driver.stopped[1]);

    // 手动禁止可立即关闭输出，解除禁止仍服从SA状态。
    Command::setOutputInhibited(true);
    tick(now, manual, switches, command, allocation, mixing);
    assert(driver.stopped[0] && driver.stopped[1]);
    Command::setOutputInhibited(false);
    tick(now, manual, switches, command, allocation, mixing);
    assert(!driver.stopped[0] && !driver.stopped[1]);

    // 运行中换安全开关编号必须重新观察新开关OFF，不可沿用旧互锁。
    assert(params.set(params.find("CA_ARM_SW"), int32_t{2}));
    switches.positions[1] = ManualControlSwitches::POSITION_ON;
    tick(now, manual, switches, command, allocation, mixing);
    assert(driver.stopped[0] && driver.stopped[1]);
    switches.positions[1] = ManualControlSwitches::POSITION_OFF;
    tick(now, manual, switches, command, allocation, mixing);
    switches.positions[1] = ManualControlSwitches::POSITION_ON;
    tick(now, manual, switches, command, allocation, mixing);
    assert(!driver.stopped[0] && !driver.stopped[1]);

    // 真实驱动回调失败时，必须停止两类执行器；下次更新可以重试。
    driver.failNext = true;
    const unsigned stops = driver.stopCalls[1];
    assert(!tick(now, manual, switches, command, allocation, mixing));
    assert(driver.stopped[0] && driver.stopped[1] && driver.stopCalls[1] > stops);
    assert(tick(now, manual, switches, command, allocation, mixing));
    assert(!driver.stopped[0] && !driver.stopped[1]);

    // 新鲜安全心跳不能掩盖分配器停止后过期的执行器消息。
    now += 600000;
    publishInput(now, manual, switches);
    command.update(now);
    assert(mixing.update(now));
    assert(driver.stopped[0] && driver.stopped[1]);
    tick(now, manual, switches, command, allocation, mixing);

    // command停止更新，即使分配器继续发布也不能保持使能。
    now += 600000;
    publishOutputs(now, manual, allocation);
    assert(mixing.update(now));
    assert(driver.stopped[0] && driver.stopped[1]);
    tick(now, manual, switches, command, allocation, mixing);

    // 遥控不再更新时，command自身必须下发失锁消息。
    now += 600000;
    command.update(now);
    ActuatorArmed status{};
    assert(actuatorArmedTopic().copy(status) && !status.armed && !status.valid);
    mixing.update(now);
    assert(driver.stopped[0] && driver.stopped[1]);
    tick(now, manual, switches, command, allocation, mixing);

    // 直接注入越界、非有限输出，绕过分配器也不能进入驱动。
    ActuatorMotors motors{};
    assert(actuatorMotorsTopic().copy(motors));
    motors.control[0] = 2.0f;
    actuatorMotorsTopic().publish(motors);
    mixing.update(now);
    assert(driver.outputs[0][0] == 1.0f);
    motors.control[0] = NAN;
    actuatorMotorsTopic().publish(motors);
    mixing.update(now);
    assert(driver.stopped[0] && driver.stopped[1]);
    motors.control[0] = 0.0f;
    motors.count = 3;
    actuatorMotorsTopic().publish(motors);
    mixing.update(now);
    assert(driver.stopped[0] && driver.stopped[1]);

    assert(params.set(params.find("CA_ARM_SW"), int32_t{99}));
    tick(now, manual, switches, command, allocation, mixing);
    assert(actuatorArmedTopic().copy(status) && !status.valid && !status.armed);
    puts("command -> allocation -> MixingOutput -> virtual driver test passed");
  }

private:
  /** 使用同一个采样时间发布摇杆与开关。 */
  static void publishInput(uint64_t now, ManualControl &manual,
                            ManualControlSwitches &switches)
  {
    manual.timestamp = manual.timestampSample = now;
    switches.timestamp = switches.timestampSample = now;
    assert(manualControlTopic().publish(manual));
    assert(manualControlSwitchesTopic().publish(switches));
  }
  /** 使用真实矩阵算法发布两类执行器目标。 */
  static void publishOutputs(uint64_t now, ManualControl &manual, Allocation &allocation)
  {
    manual.timestamp = manual.timestampSample = now;
    ActuatorMotors motors{};
    ActuatorServos servos{};
    allocation.allocate(manual, motors, servos);
    assert(actuatorMotorsTopic().publish(motors));
    assert(actuatorServosTopic().publish(servos));
  }
  /** 前进一个20毫秒周期，所有通信都通过公开topic接口完成。 */
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
};

/** 执行主机端完整安全输出链路验收。 */
int main()
{
  OutputChainTest::run();
  return 0;
}
