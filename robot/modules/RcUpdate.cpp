/**
 * @file RcUpdate.cpp
 * @brief 实现遥控校准、功能映射、信号有效性判断和手动控制 topic 发布。
 */

// NuttX Make对命令源文件定义main宏；显式命名入口无需重命名，
// 取消该宏以避免改写ModuleBase::main等C++成员名称。
#ifdef main
#undef main
#endif

#include "robot/modules/RcUpdate.hpp"

#include "robot/os/Clock.hpp"

#include <stdio.h>

/** 初始化 uORB 端点和 RC 链路状态。 */
RcUpdate::RcUpdate()
    : _inputSubscription(inputRcTopic()),
      _parameterSubscription(parameterUpdateTopic()),
      _channelsPublication(rcChannelsTopic()),
      _manualPublication(manualControlTopic()),
      _switchesPublication(manualControlSwitchesTopic()),
      _lastInputTimestamp(0),
      _previousChannelCount(0),
      _stableFrameCount(0),
      _processedFrames(0),
      _signalLost(true),
      _timeoutPublished(false)
{
}

/** 标记模块正在启动并创建 rc_update 线程。 */
int RcUpdate::task_spawn(int argc, char *argv[])
{
  const int taskId = os::Task::spawn("rc_update", 110, 4096,
                                     &run_trampoline, argc, argv)
  if (taskId < 0)
    {
      __atomic_store_n(&_taskId, -1, __ATOMIC_RELEASE);
      return -1;
    }

  __atomic_store_n(&_taskId, taskId, __ATOMIC_RELEASE);
  return wait_until_running();
}

/** 创建不依赖硬件文件描述符的 rc_update 对象。 */
RcUpdate *RcUpdate::instantiate(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  return new RcUpdate();
}

/** 打印 rc_update 的标准生命周期命令。 */
int RcUpdate::print_usage(const char *reason)
{
  if (reason != nullptr)
    {
      fprintf(stderr, "%s\n", reason);
    }

  printf("usage: rc_update {start|stop|status}\n");
  return reason == nullptr ? 0 : -1;
}

/** 输出处理统计、归一化摇杆、开关位置和最新信号状态。 */
int RcUpdate::print_status()
{
  printf("running\n");
  printf("processed frames: %lu\n",
         static_cast<unsigned long>(
             __atomic_load_n(&_processedFrames, __ATOMIC_ACQUIRE)));
  printf("calibration: %s\n", _config.valid() ? "valid" : "invalid");
  printf("signal lost: %s\n",
         __atomic_load_n(&_signalLost, __ATOMIC_ACQUIRE) ? "yes" : "no");

  ManualControl manual;
  if (manualControlTopic().copy(manual))
    {
      printf("manual valid: %s\n", manual.valid ? "yes" : "no");
      printf("roll/pitch/yaw/throttle: %.3f %.3f %.3f %.3f\n",
             static_cast<double>(manual.roll),
             static_cast<double>(manual.pitch),
             static_cast<double>(manual.yaw),
             static_cast<double>(manual.throttle));
    }
  else
    {
      printf("latest manual_control: unavailable\n");
    }

  ManualControlSwitches switches;
  if (manualControlSwitchesTopic().copy(switches))
    {
      printf("switches:");
      for (uint8_t index = 0; index < switches.switchCount; ++index)
        {
          printf(" %u", static_cast<unsigned>(switches.positions[index]));
        }
      printf("\n");
    }

  return 0;
}

/** 以 100 Hz 轮询轻量 uORB，不需要 PX4 任务队列。 */
void RcUpdate::run()
{
  _lastInputTimestamp = os::Clock::nowMicroseconds();

  while (!should_exit())
    {
      const uint64_t now = os::Clock::nowMicroseconds();
      ParameterUpdate parameterUpdate;
      if (_parameterSubscription.update(parameterUpdate))
        {
          (void)_config.refresh();
        }

      InputRc input;
      if (_inputSubscription.update(input))
        {
          processInput(input, now);
        }
      else
        {
          publishTimeoutIfNeeded(now);
        }

      os::Clock::sleepMilliseconds(10);
    }
}

/** 应用 MIN/TRIM/MAX/DEADZONE/REV，并生成通道、摇杆和开关消息。 */
void RcUpdate::processInput(const InputRc &input, uint64_t now)
{
  _lastInputTimestamp = now;
  _timeoutPublished = false;

  if (input.channelCount == _previousChannelCount &&
      input.channelCount >= 4)
    {
      if (_stableFrameCount < 3)
        {
          ++_stableFrameCount;
        }
    }
  else
    {
      _stableFrameCount = 0;
      _previousChannelCount = input.channelCount;
    }

  const bool signalLost = input.rcLost || input.rcFailsafe ||
                          input.channelCount < 4;
  __atomic_store_n(&_signalLost, signalLost, __ATOMIC_RELEASE);

  RcChannels channels;
  channels.timestamp = now;
  channels.timestampLastValid = signalLost ? 0 : input.timestampLastSignal;
  channels.channelCount = input.channelCount > InputRc::MAX_CHANNELS
                              ? InputRc::MAX_CHANNELS
                              : input.channelCount;
  channels.frameDropCount = input.lostFrameCount;
  channels.signalLost = signalLost;

  for (uint8_t function = 0; function < RcChannels::FUNCTION_COUNT;
       ++function)
    {
      const uint8_t mappedChannel = _config.functionChannel(function);
      channels.function[function] =
          mappedChannel < InputRc::MAX_CHANNELS
              ? static_cast<int8_t>(mappedChannel)
              : static_cast<int8_t>(-1);
    }

  for (uint8_t channel = 0; channel < channels.channelCount; ++channel)
    {
      channels.channels[channel] =
          normalizeRcChannel(input.values[channel],
                             _config.calibration(channel));
    }

  _channelsPublication.publish(channels);

  const bool controlsValid = !signalLost && _config.valid() &&
                             _stableFrameCount >= 2;

  ManualControl manual;
  manual.timestamp = now;
  manual.timestampSample = input.timestampLastSignal;
  manual.roll = functionValue(channels, RcChannels::FUNCTION_ROLL, 0.0f);
  manual.pitch = functionValue(channels, RcChannels::FUNCTION_PITCH, 0.0f);
  manual.yaw = functionValue(channels, RcChannels::FUNCTION_YAW, 0.0f);
  manual.throttle =
      functionValue(channels, RcChannels::FUNCTION_THROTTLE, -1.0f);
  manual.valid = controlsValid;
  _manualPublication.publish(manual);

  ManualControlSwitches switches;
  switches.timestamp = now;
  switches.timestampSample = input.timestampLastSignal;
  switches.switchCount = RcChannels::SWITCH_COUNT;
  switches.valid = controlsValid;

  for (uint8_t switchIndex = 0;
       switchIndex < RcChannels::SWITCH_COUNT; ++switchIndex)
    {
      const float value = functionValue(
          channels,
          static_cast<uint8_t>(RcChannels::FUNCTION_SWITCH_FIRST + switchIndex),
          0.0f);
      switches.positions[switchIndex] = rcSwitchPosition(value);
    }

  _switchesPublication.publish(switches);
  __atomic_add_fetch(&_processedFrames, 1U, __ATOMIC_RELAXED);
}

/** 没有新 input_rc 达 500 ms 时向上层发布无效控制状态。 */
void RcUpdate::publishTimeoutIfNeeded(uint64_t now)
{
  if (_timeoutPublished || _lastInputTimestamp == 0 ||
      now - _lastInputTimestamp <= InputTimeoutMicroseconds)
    {
      return;
    }

  __atomic_store_n(&_signalLost, true, __ATOMIC_RELEASE);
  _stableFrameCount = 0;

  RcChannels channels;
  channels.timestamp = now;
  channels.signalLost = true;
  for (uint8_t function = 0; function < RcChannels::FUNCTION_COUNT;
       ++function)
    {
      channels.function[function] = -1;
    }
  _channelsPublication.publish(channels);

  ManualControl manual;
  manual.timestamp = now;
  manual.throttle = -1.0f;
  manual.valid = false;
  _manualPublication.publish(manual);

  ManualControlSwitches switches;
  switches.timestamp = now;
  switches.valid = false;
  _switchesPublication.publish(switches);
  _timeoutPublished = true;
}

/** 根据功能映射安全读取归一化通道值。 */
float RcUpdate::functionValue(const RcChannels &channels, uint8_t function,
                              float defaultValue) const
{
  if (function >= RcChannels::FUNCTION_COUNT)
    {
      return defaultValue;
    }

  const int8_t channel = channels.function[function];
  if (channel < 0 || static_cast<uint8_t>(channel) >= channels.channelCount)
    {
      return defaultValue;
    }

  return channels.channels[channel];
}

/** 提供NSH的rc_update命令入口，转交ModuleBase统一处理生命周期。 */
extern "C" int rc_update_main(int argc, char *argv[])
{
  return RcUpdate::main(argc, argv);
}
