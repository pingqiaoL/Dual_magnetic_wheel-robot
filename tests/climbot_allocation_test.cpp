/**
 * @file climbot_allocation_test.cpp
 * @brief 验证 Climbot 默认只用前转向偏航，并允许参数启用后转向。
 */

#include "robot/control/Allocation.hpp"
#include "robot/control/ActuatorEffectivenessDualMagneticWheel.hpp"
#include "robot/control/ArmSafety.hpp"

#include "robot/params/ParamManager.hpp"

#include <assert.h>
#include <math.h>
#include <stdio.h>

int main()
{
  ArmSafety safety;
  assert(!safety.update(true, ManualControlSwitches::POSITION_ON));
  assert(safety.waitingForOff());
  assert(!safety.update(true, ManualControlSwitches::POSITION_OFF));
  assert(!safety.waitingForOff());
  assert(!safety.update(true, ManualControlSwitches::POSITION_MIDDLE));
  assert(safety.update(true, ManualControlSwitches::POSITION_ON));
  assert(safety.update(true, ManualControlSwitches::POSITION_ON));
  assert(!safety.update(false, ManualControlSwitches::POSITION_ON));
  assert(!safety.waitingForOff());
  assert(safety.update(true, ManualControlSwitches::POSITION_ON));
  assert(!safety.update(true, ManualControlSwitches::POSITION_OFF));
  assert(safety.update(true, ManualControlSwitches::POSITION_ON));

  ActuatorEffectivenessDualMagneticWheel effectiveness;
  Allocation allocation(effectiveness);
  ManualControl manual{};
  manual.timestamp = 100;
  manual.timestampSample = 90;
  manual.throttle = 0.4f;
  manual.yaw = -0.5f;
  manual.valid = true;

  ActuatorMotors motors{};
  ActuatorServos servos{};
  allocation.allocate(manual, motors, servos);
  assert(motors.count == 2 && servos.count == 2);
  assert(fabsf(motors.control[0] - 0.4f) < 0.0001f);
  assert(fabsf(motors.control[1] - 0.4f) < 0.0001f);
  assert(fabsf(servos.control[0] + 0.5f) < 0.0001f);
  assert(servos.control[1] == 0.0f);

  ParamManager &parameters = ParamManager::instance();
  const ParamHandle rearYaw = parameters.find("CA_YAW_R");
  assert(rearYaw != ParamInvalid && parameters.set(rearYaw, -1.0f));
  assert(allocation.updateParameters());
  motors = ActuatorMotors{};
  servos = ActuatorServos{};
  allocation.allocate(manual, motors, servos);
  assert(fabsf(servos.control[1] - 0.5f) < 0.0001f);

  motors = ActuatorMotors{};
  servos = ActuatorServos{};
  manual.valid = false;
  allocation.allocate(manual, motors, servos);
  assert(!motors.valid && motors.control[0] == 0.0f);
  assert(!servos.valid && servos.control[0] == 0.0f);

  manual.valid = true;
  manual.throttle = 2.0f;
  allocation.allocate(manual, motors, servos);
  assert(motors.control[0] == 1.0f);
  manual.yaw = NAN;
  allocation.allocate(manual, motors, servos);
  assert(!motors.valid && !servos.valid);

  const ParamHandle throttleFront = parameters.find("CA_THR_F");
  assert(parameters.setDefault(throttleFront, 0.8f));
  float value = 0.0f;
  assert(parameters.get(throttleFront, value) && value == 0.8f);
  assert(parameters.setDefault(rearYaw, 0.25f));
  assert(parameters.get(rearYaw, value) && value == -1.0f);
  assert(parameters.reset(rearYaw));
  assert(parameters.get(rearYaw, value) && value == 0.25f);
  printf("climbot allocation test passed\n");
  return 0;
}
