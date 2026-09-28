/**
 * @file MixingOutput.cpp
 * @brief 实现OutputFunction路由、周期刷新、硬故障归零和物理失能。
 */

#include "robot/output/MixingOutput.hpp"
#include "robot/os/Clock.hpp"

#include <math.h>

static constexpr uint64_t TopicTimeoutUs = 500000ULL;
static constexpr uint64_t OutputPeriodUs = 20000ULL;
static constexpr uint64_t NeutralBeforeDisableUs = 500000ULL;

MixingOutput::MixingOutput(ModuleParams *parent, OutputInterface &interface,
                           const char *const *functionParams,
                           uint8_t maxOutputs)
    : ModuleParams(parent), _interface(interface),
      _functionParamNames(functionParams),
      _maxOutputs(maxOutputs <= MaxOutputs ? maxOutputs : MaxOutputs),
      _paramFailAction(this, "OUT_FAIL_ACT", 2),
      _armedSubscription(actuatorArmedTopic()),
      _motorsSubscription(actuatorMotorsTopic()),
      _servosSubscription(actuatorServosTopic())
{
  for (uint8_t index = 0; index < MaxOutputs; ++index)
    { _functionHandles[index] = ParamInvalid; }
  (void)updateParamsImpl();
}

bool MixingOutput::fresh(uint64_t timestamp, uint64_t now)
{
  return timestamp != 0 && timestamp <= now &&
         now - timestamp <= TopicTimeoutUs;
}

float MixingOutput::constrainUnit(float value)
{
  return value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
}

bool MixingOutput::updateParamsImpl()
{
  bool valid = _functionParamNames != nullptr;
  ParamManager &manager = ParamManager::instance();
  for (uint8_t channel = 0; channel < _maxOutputs; ++channel)
    {
      if (_functionHandles[channel] == ParamInvalid &&
          _functionParamNames != nullptr)
        { _functionHandles[channel] = manager.find(_functionParamNames[channel]); }
      int32_t value = 0;
      if (_functionHandles[channel] == ParamInvalid ||
          !manager.get(_functionHandles[channel], value))
        {
          _functions[channel] = OutputFunction::Disabled;
          valid = false;
          continue;
        }
      const OutputFunction function = static_cast<OutputFunction>(value);
      if (function != OutputFunction::Disabled &&
          !outputFunctionIsMotor(function) && !outputFunctionIsServo(function))
        {
          _functions[channel] = OutputFunction::Disabled;
          valid = false;
        }
      else
        { _functions[channel] = function; }
    }
  return valid;
}

bool MixingOutput::isFunctionSet(uint8_t channel) const
{
  return channel < _maxOutputs &&
         _functions[channel] != OutputFunction::Disabled;
}

OutputFunction MixingOutput::function(uint8_t channel) const
{
  return channel < _maxOutputs ? _functions[channel]
                               : OutputFunction::Disabled;
}

bool MixingOutput::hasAssignedFunctions() const
{
  for (uint8_t channel = 0; channel < _maxOutputs; ++channel)
    { if (isFunctionSet(channel)) { return true; } }
  return false;
}

bool MixingOutput::sourceDataValid(uint64_t now) const
{
  bool needsMotors = false;
  bool needsServos = false;
  for (uint8_t channel = 0; channel < _maxOutputs; ++channel)
    {
      needsMotors = needsMotors || outputFunctionIsMotor(_functions[channel]);
      needsServos = needsServos || outputFunctionIsServo(_functions[channel]);
    }
  // timestampSample保留原始RC采样时间；RobotControl已经根据独立的
  // Failsafe标志生成安全目标。输出层只检查分配器是否仍在发布新鲜消息。
  if (needsMotors && (!_motors.valid || !fresh(_motors.timestamp, now)))
    { return false; }
  if (needsServos && (!_servos.valid || !fresh(_servos.timestamp, now)))
    { return false; }

  for (uint8_t channel = 0; channel < _maxOutputs; ++channel)
    {
      const OutputFunction assigned = _functions[channel];
      if (assigned == OutputFunction::Disabled) { continue; }
      const uint8_t index = outputFunctionIndex(assigned);
      if (outputFunctionIsMotor(assigned))
        {
          if (index >= _motors.count || index >= ActuatorMotors::MAX_CONTROLS ||
              !isfinite(_motors.control[index])) { return false; }
        }
      else if (index >= _servos.count || index >= ActuatorServos::MAX_CONTROLS ||
               !isfinite(_servos.control[index])) { return false; }
    }
  return true;
}

bool MixingOutput::fillPhysicalOutputs(float *outputs) const
{
  if (outputs == nullptr) { return false; }
  for (uint8_t channel = 0; channel < _maxOutputs; ++channel)
    {
      const OutputFunction assigned = _functions[channel];
      if (assigned == OutputFunction::Disabled)
        { outputs[channel] = 0.0f; continue; }
      const uint8_t index = outputFunctionIndex(assigned);
      outputs[channel] = constrainUnit(outputFunctionIsMotor(assigned)
          ? _motors.control[index] : _servos.control[index]);
    }
  return true;
}

bool MixingOutput::stopOutputs(uint64_t now)
{
  const bool stopped = _interface.updateOutputs(true, nullptr, 0, now);
  _stopped = stopped;
  _lastOutput = now;
  return stopped;
}

bool MixingOutput::update(uint64_t now)
{
  const bool armChanged = _armedSubscription.update(_armed);
  const bool motorsChanged = _motorsSubscription.update(_motors);
  const bool servosChanged = _servosSubscription.update(_servos);
  if (now == 0) { now = os::Clock::nowMicroseconds(); }

  const bool disarm = !_armed.armed || _armed.lockdown ||
                      !fresh(_armed.timestamp, now);
  if (disarm || !hasAssignedFunctions())
    {
      _dataFaultSince = 0;
      if (_stopped && !armChanged && _lastOutput != 0 &&
          now - _lastOutput < OutputPeriodUs) { return true; }
      return stopOutputs(now);
    }

  const bool dataFault = !sourceDataValid(now);
  if (dataFault && _dataFaultSince == 0) { _dataFaultSince = now; }
  if (!dataFault) { _dataFaultSince = 0; }

  const int32_t configuredAction = _paramFailAction.get();
  const int32_t faultAction = configuredAction >= 0 && configuredAction <= 2
                                  ? configuredAction : 0;
  if (dataFault && (faultAction == 0 ||
      (faultAction == 2 && now - _dataFaultSince >= NeutralBeforeDisableUs)))
    { return stopOutputs(now); }

  if (!armChanged && !motorsChanged && !servosChanged && !_stopped &&
      _lastOutput != 0 && now - _lastOutput < OutputPeriodUs)
    { return true; }

  float outputs[MaxOutputs]{};
  if (!dataFault && !fillPhysicalOutputs(outputs)) { return stopOutputs(now); }
  if (!_interface.updateOutputs(false, outputs, _maxOutputs, now))
    {
      (void)stopOutputs(now);
      return false;
    }

  _stopped = false;
  _lastOutput = now;
  return true;
}
