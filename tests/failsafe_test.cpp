/** @file failsafe_test.cpp
 * @brief 验证Failsafe只发布标志，RobotControl应用目标策略。
 */
#include "robot/modules/Failsafe.hpp"
#include "robot/modules/RobotControl.hpp"
#include "robot/common/CommandRouter.hpp"
#include "robot/params/ParamManager.hpp"
#include <assert.h>
#include <math.h>
#include <stdio.h>

/** 主机单元测试不创建NuttX任务。 */
int os::Task::spawn(const char *, int, size_t, Entry, int, char *[])
{
  return -1;
}

/** 主机单元测试不删除NuttX任务。 */
int os::Task::terminate(int)
{
  return -1;
}

/** 生命周期扩展命令不属于本测试范围。 */
int CommandRouter::dispatch(const char *, int, char *[])
{
  return -1;
}

int main()
{
  ParamManager &params = ParamManager::instance();
  assert(params.set(params.find("FS_RC_TMO"), 0.5f));
  assert(params.set(params.find("FS_MOT_ACT"), int32_t{1}));
  assert(params.set(params.find("FS_STR_ACT"), int32_t{0}));
  Failsafe failsafe;
  RobotControl control;
  FailsafeStatus status{};
  RobotControlSetpoint setpoint{};
  uint64_t now = 1000000;

  failsafe.update(now);
  assert(failsafeStatusTopic().copy(status));
  assert(status.active);
  // Failsafe自身不允许发布或转发RobotControlSetpoint。
  assert(!robotControlSetpointTopic().copy(setpoint));
  control.update(now);
  assert(robotControlSetpointTopic().copy(setpoint));
  assert(!setpoint.valid && setpoint.failsafe);

  ManualControl manual{};
  manual.timestamp = manual.timestampSample = now;
  manual.throttle = 0.4f;
  manual.yaw = -0.5f;
  manual.valid = true;
  assert(manualControlTopic().publish(manual));
  failsafe.update(now);
  assert(failsafeStatusTopic().copy(status) && !status.active);
  control.update(now);
  assert(robotControlSetpointTopic().copy(setpoint));
  assert(setpoint.valid && !setpoint.failsafe);
  assert(fabsf(setpoint.speed - 0.4f) < 0.0001f);
  assert(fabsf(setpoint.steering + 0.5f) < 0.0001f);

  now += 600000;
  failsafe.update(now);
  assert(failsafeStatusTopic().copy(status) && status.active);
  control.update(now);
  assert(robotControlSetpointTopic().copy(setpoint));
  assert(setpoint.valid && setpoint.failsafe);
  assert(setpoint.speed == 0.0f);
  assert(fabsf(setpoint.steering + 0.5f) < 0.0001f);

  manual.timestamp = manual.timestampSample = now;
  manual.throttle = -0.2f;
  manual.yaw = 0.25f;
  assert(manualControlTopic().publish(manual));
  failsafe.update(now);
  control.update(now);
  assert(robotControlSetpointTopic().copy(setpoint));
  assert(setpoint.valid && !setpoint.failsafe);
  assert(fabsf(setpoint.speed + 0.2f) < 0.0001f);
  assert(fabsf(setpoint.steering - 0.25f) < 0.0001f);

  // 参数更新可把丢失策略切换为动力固定值和转向回中。
  assert(params.set(params.find("FS_MOT_ACT"), int32_t{2}));
  assert(params.set(params.find("FS_MOT_VAL"), -0.1f));
  assert(params.set(params.find("FS_STR_ACT"), int32_t{1}));
  now += 600000;
  failsafe.update(now);
  control.update(now);
  assert(robotControlSetpointTopic().copy(setpoint));
  assert(setpoint.valid && setpoint.failsafe);
  assert(fabsf(setpoint.speed + 0.1f) < 0.0001f);
  assert(setpoint.steering == 0.0f);
  puts("failsafe flag and RobotControl policy test passed");
  return 0;
}
