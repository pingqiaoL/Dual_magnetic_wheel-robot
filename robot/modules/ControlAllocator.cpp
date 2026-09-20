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
  : _modelId(modelId), _effectiveness(effectiveness), _allocation(*effectiveness),
    _manualSubscription(manualControlTopic()),
    _parameterSubscription(parameterUpdateTopic()),
    _motorsPublication(actuatorMotorsTopic()),
    _servosPublication(actuatorServosTopic())
{
  updateParameters();
}

/** 释放已退出任务持有的构型对象。 */
ControlAllocator::~ControlAllocator()
{
  delete _effectiveness;
}

/** 将模块主循环交给独立NuttX任务。 */
int ControlAllocator::task_spawn(int argc, char *argv[])
{
  const int id = os::Task::spawn("control_allocator", 105, 4096,
                               &run_trampoline, argc, argv);
  __atomic_store_n(&_taskId, id < 0 ? -1 : id, __ATOMIC_RELEASE);
  return id < 0 ? -1 : wait_until_running();
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
  os::LockGuard guard(_stateMutex);
  if (!guard.locked()) { return -1; }
  printf("running\nconfiguration: %s\nCA_AIRFRAME: %ld\n",
         _effectiveness->name(), static_cast<long>(_modelId));
  printf("motors/servos: %u/%u\nmodel valid: %s\nallocations: %lu\n",
         _effectiveness->motorCount(), _effectiveness->servoCount(),
         _modelValid ? "yes" : "no", static_cast<unsigned long>(_allocationCount));
  printf("arming status: use command status\n");
  return 0;
}

/** 运行中改变编号先发布无效输出，停止并重新启动模块才创建新模型。 */
void ControlAllocator::updateParameters()
{
  int32_t selected = 0;
  const bool match = ParamManager::instance().get("CA_AIRFRAME", selected) &&
                     selected == _modelId;
  if (!match && _modelValid)
    { printf("WARNING [control_allocator] model changed; restart allocator\n"); }
  _modelValid = match && _allocation.updateParameters();
}

/** 本任务只计算发布，解锁判断属于command和MixingOutput。 */
void ControlAllocator::run()
{
  while (!should_exit())
    {
      {
        os::LockGuard guard(_stateMutex);
        if (guard.locked())
          {
            ParameterUpdate update{};
            if (_parameterSubscription.update(update)) { updateParameters(); }
            (void)_manualSubscription.update(_manual);
            const uint64_t now = os::Clock::nowMicroseconds();
            ManualControl input = _manual;
            input.valid = input.valid && _modelValid && input.timestampSample != 0 &&
                          input.timestampSample <= now &&
                          now - input.timestampSample <= 500000ULL;
            ActuatorMotors motors{};
            ActuatorServos servos{};
            _allocation.allocate(input, motors, servos);
            motors.timestamp = servos.timestamp = now;
            (void)_motorsPublication.publish(motors);
            (void)_servosPublication.publish(servos);
            ++_allocationCount;
          }
      }
      os::Clock::sleepMilliseconds(20);
    }
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
