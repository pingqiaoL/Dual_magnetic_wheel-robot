/**
 * @file MixingOutput.hpp
 * @brief 将逻辑执行器Topic映射成某个驱动的物理输出数组并执行最后安全检查。
 */

#pragma once

#include "robot/orb/Topics.hpp"
#include "robot/output/OutputFunction.hpp"
#include "robot/output/OutputInterface.hpp"
#include "robot/params/ModuleParams.hpp"
#include "robot/params/Param.hpp"

/** CAN及未来PWM/UART驱动共用的输出处理器，不创建独立线程。 */
class MixingOutput : public ModuleParams
{
public:
  static constexpr uint8_t MaxOutputs = 8;

  /** functionParams按物理通道给出FUNC参数名，数组生命周期必须覆盖本对象。 */
  MixingOutput(ModuleParams *parent, OutputInterface &interface,
               const char *const *functionParams, uint8_t maxOutputs);

  bool update(uint64_t now = 0);
  bool isFunctionSet(uint8_t channel) const;
  OutputFunction function(uint8_t channel) const;
  uint8_t maxOutputs() const { return _maxOutputs; }

protected:
  bool updateParamsImpl() override;

private:
  static bool fresh(uint64_t timestamp, uint64_t now);
  static float constrainUnit(float value);
  bool stopOutputs(uint64_t now);
  bool sourceDataValid(uint64_t now) const;
  bool fillPhysicalOutputs(float *outputs) const;
  bool hasAssignedFunctions() const;

  OutputInterface &_interface;
  const char *const *_functionParamNames;
  const uint8_t _maxOutputs;
  ParamHandle _functionHandles[MaxOutputs]{};
  OutputFunction _functions[MaxOutputs]{};
  ParamInt _paramFailAction;
  uorb::Subscription<ActuatorArmed> _armedSubscription;
  uorb::Subscription<ActuatorMotors> _motorsSubscription;
  uorb::Subscription<ActuatorServos> _servosSubscription;
  ActuatorArmed _armed{};
  ActuatorMotors _motors{};
  ActuatorServos _servos{};
  uint64_t _lastOutput{0};
  uint64_t _dataFaultSince{0};
  bool _stopped{true};
};
