/** @file ControlAllocator.cpp
 * @brief 创建构型子类、更新分配矩阵并周期发布电机与舵机目标。
 */
#ifdef main
#undef main
#endif
#include "robot/modules/ControlAllocator.hpp"
#include "robot/control/ActuatorEffectivenessDualMagneticWheel.hpp"
#include "robot/params/ParamManager.hpp"
#include "robot/os/Clock.hpp"
#include <stdio.h>
#include <string.h>

/** 模型归模块所有；Allocation只引用同一个模型，不再复制构型信息。 */
ControlAllocator::ControlAllocator(int32_t modelId, ActuatorEffectiveness *effectiveness)
  : WorkQueueModuleBase<ControlAllocator>(robot_wq::ControlQueue(), 20000),
    ModuleParams(nullptr), _modelId(modelId),
    _paramAirframe(this, "CA_AIRFRAME", 1),
    _effectiveness(effectiveness), _allocation(*effectiveness),
    _setpointSubscription(robotControlSetpointTopic(), *this),
    _parameterSubscription(parameterUpdateTopic(), *this),
    _motorsPublication(actuatorMotorsTopic()),
    _servosPublication(actuatorServosTopic())
{
  _effectiveness->setParent(this);
  (void)updateParams();
  updateParameters();
}

/** 释放已退出任务持有的构型对象。 */
ControlAllocator::~ControlAllocator()
{
  delete _effectiveness;
}



/** 每个编号对应一个构型子类，不支持的编号拒绝启动。 */
ActuatorEffectiveness *ControlAllocator::createEffectiveness(int32_t modelId)
{
  switch (modelId)
    {
      case 1: return new ActuatorEffectivenessDualMagneticWheel();
      default:
        fprintf(stderr, "unsupported CA_AIRFRAME: %ld\n", static_cast<long>(modelId));
        return nullptr;
    }
}

/** 模型编号由参数决定；旧-c climbot只用于检查兼容性。 */
ControlAllocator *ControlAllocator::instantiate(int argc, char *argv[])
{
  int32_t modelId = 0;
  if (!ParamManager::instance().get("CA_AIRFRAME", modelId)) { return nullptr; }
  for (int index = 1; index < argc; ++index)
    {
      if (strcmp(argv[index], "-c") != 0 || index + 1 >= argc ||
          strcmp(argv[++index], "climbot") != 0 || modelId != 1)
        { print_usage("unsupported configuration option"); return nullptr; }
    }
  ActuatorEffectiveness *effectiveness = createEffectiveness(modelId);
  if (effectiveness == nullptr) { return nullptr; }
  ControlAllocator *module = new ControlAllocator(modelId, effectiveness);
  if (module == nullptr) { delete effectiveness; }
  return module;
}

/** 打印生命周期命令和模型选择参数。 */
int ControlAllocator::print_usage(const char *reason)
{
  if (reason != nullptr) { fprintf(stderr, "%s\n", reason); }
  printf("usage: control_allocator {start [-c climbot]|stop|status}\n");
  printf("model selection: CA_AIRFRAME (1=dual_magneticwheel)\n");
  return reason == nullptr ? 0 : -1;
}

/** 状态读取与分配参数刷新使用同一个状态锁。 */
int ControlAllocator::print_status()
{
  bool valid;
  uint32_t count;
  int32_t selected;
  {
    os::LockGuard guard(_stateMutex);
    if (!guard.locked()) { return -1; }
    valid = _modelValid;
    count = _allocationCount;
    selected = _paramAirframe.get();
  }
  printf("running\nconfiguration: %s\nCA_AIRFRAME: %ld\n",
         _effectiveness->name(), static_cast<long>(_modelId));
  printf("motors/servos: %u/%u\nmodel valid: %s\nallocations: %lu\n",
         _effectiveness->motorCount(), _effectiveness->servoCount(),
         valid ? "yes" : "no", static_cast<unsigned long>(count));
  if (selected != _modelId) { printf("model changed; restart allocator\n"); }
  printf("arming status: use command status\n");
  return 0;
}

/** 运行中改变编号先发布无效输出，停止并重新启动模块才创建新模型。 */
void ControlAllocator::updateParameters()
{
  (void)updateParams();
  const int32_t selected = _paramAirframe.get();
  const bool match = selected == _modelId;
  _modelValid = match && _allocation.updateParameters();
}

/** 本任务只执行构型分配；RC丢失目标策略属于RobotControl。 */
bool ControlAllocator::initWorkQueue()
{
  return _setpointSubscription.registerCallback() &&
         _parameterSubscription.registerCallback();
}

void ControlAllocator::unregisterCallbacks()
{
  _setpointSubscription.unregisterCallback();
  _parameterSubscription.unregisterCallback();
}

void ControlAllocator::Run()
{
  os::LockGuard guard(_stateMutex);
  if (!guard.locked()) { return; }
  ParameterUpdate update{};
  if (_parameterSubscription.update(update)) { updateParameters(); }
  const uint64_t now = os::Clock::nowMicroseconds();
  (void)_setpointSubscription.update(_setpoint);
  const bool freshSetpoint = _setpoint.timestamp != 0 &&
                             _setpoint.timestamp <= now &&
                             now - _setpoint.timestamp <= 500000ULL;
  ManualControl input{};
  input.timestamp = now;
  input.timestampSample = _setpoint.timestampSample;
  input.throttle = _setpoint.speed;
  input.yaw = _setpoint.steering;
  input.valid = _modelValid && _setpoint.valid && freshSetpoint;
  ActuatorMotors motors{};
  ActuatorServos servos{};
  _allocation.allocate(input, motors, servos);
  motors.timestamp = servos.timestamp = now;
  (void)_motorsPublication.publish(motors);
  (void)_servosPublication.publish(servos);
  ++_allocationCount;
}

void ControlAllocator::onWorkQueueStopped()
{
  // 退出立即发布无效输出，避免等待缓存超时才停止驱动。
  ActuatorMotors motors{};
  ActuatorServos servos{};
  motors.timestamp = servos.timestamp = os::Clock::nowMicroseconds();
  (void)_motorsPublication.publish(motors);
  (void)_servosPublication.publish(servos);
}

/** NSH入口转交模板父类管理生命周期。 */
extern "C" int control_allocator_main(int argc, char *argv[])
{
  return ControlAllocator::main(argc, argv);
}
