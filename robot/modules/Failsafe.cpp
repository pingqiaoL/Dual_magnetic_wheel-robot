/** @file Failsafe.cpp
 * @brief 实现RC丢失检测并发布独立故障标志。
 */
#ifdef main
#undef main
#endif
#include "robot/modules/Failsafe.hpp"
#include "robot/os/Clock.hpp"
#include <math.h>
#include <stdio.h>

/** 建立手动输入订阅和Failsafe状态发布。 */
Failsafe::Failsafe()
  : ModuleParams(nullptr), _manualSubscription(manualControlTopic()),
    _parameterSubscription(parameterUpdateTopic()),
    _statusPublication(failsafeStatusTopic()),
    _paramRcTimeout(this, "FS_RC_TMO", 0.5f)
{
  (void)updateParams();
  (void)refreshPolicy();
}

/** 创建独立Failsafe任务并等待对象就绪。 */
int Failsafe::task_spawn(int argc, char *argv[])
{
  const int id = os::Task::spawn("failsafe", 105, 4096,
                                &run_trampoline, argc, argv);
  __atomic_store_n(&_taskId, id < 0 ? -1 : id, __ATOMIC_RELEASE);
  return id < 0 ? -1 : wait_until_running();
}

/** 创建Failsafe对象。 */
Failsafe *Failsafe::instantiate(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  return new Failsafe();
}

/** 输出生命周期命令。 */
int Failsafe::print_usage(const char *reason)
{
  if (reason != nullptr) { fprintf(stderr, "%s\n", reason); }
  printf("usage: failsafe {start|stop|status}\n");
  return reason == nullptr ? 0 : -1;
}

/** 判断消息字段和采样时间是否有效。 */
bool Failsafe::fresh(const ManualControl &manual, uint64_t now)
{
  return manual.valid && manual.timestampSample != 0 &&
         manual.timestampSample <= now;
}

bool Failsafe::refreshPolicy()
{
  const float timeout = _paramRcTimeout.get();
  if (!isfinite(timeout) || timeout < 0.05f || timeout > 10.0f)
    { return false; }
  _timeoutUs = static_cast<uint64_t>(timeout * 1000000.0f);
  return true;
}

/** 检测RC是否丢失，只发布active标志，不读取或生成任何控制目标。 */
void Failsafe::update(uint64_t now)
{
  os::LockGuard guard(_stateMutex);
  if (!guard.locked()) { return; }
  ManualControl received{};
  if (_manualSubscription.update(received)) { _manual = received; }
  if (now == 0) { now = os::Clock::nowMicroseconds(); }
  ParameterUpdate parameterUpdate{};
  if (_parameterSubscription.update(parameterUpdate))
    {
      (void)updateParams();
      (void)refreshPolicy();
    }
  _status.timestamp = now;
  _status.active = !(fresh(_manual, now) &&
                     now - _manual.timestampSample <= _timeoutUs);
  (void)_statusPublication.publish(_status);
}

/** 持锁输出最近一次故障检测状态。 */
int Failsafe::print_status()
{
  os::LockGuard guard(_stateMutex);
  if (!guard.locked()) { return -1; }
  printf("running\nRC lost: %s\ntimeout: %.3f s\n",
         _status.active ? "yes" : "no",
         static_cast<double>(_timeoutUs) / 1000000.0);
  return 0;
}

/** 周期发布故障标志；停止后发布active使目标生成器立即进入安全状态。 */
void Failsafe::run()
{
  while (!should_exit())
    {
      update();
      os::Clock::sleepMilliseconds(20);
    }
  FailsafeStatus stopped{};
  stopped.timestamp = os::Clock::nowMicroseconds();
  stopped.active = true;
  (void)_statusPublication.publish(stopped);
}

/** 提供NSH的failsafe命令入口。 */
extern "C" int failsafe_main(int argc, char *argv[])
{
  return Failsafe::main(argc, argv);
}
