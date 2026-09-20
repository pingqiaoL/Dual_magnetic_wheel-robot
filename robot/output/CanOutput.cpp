/**
 * @file CanOutput.cpp
 * @brief 实现 NuttX CAN 字符设备、达妙协议选择、反馈解析和 NSH 管理命令。
 */

// NuttX Make对命令源文件定义main宏；显式命名入口无需重命名，
// 取消该宏以避免改写ModuleBase::main等C++成员名称。
#ifdef main
#undef main
#endif

#include "robot/output/CanOutput.hpp"

#include "robot/os/Clock.hpp"
#include "robot/modules/Command.hpp"
#include "robot/params/ParamManager.hpp"

#include <errno.h>
#include <fcntl.h>
#include <nuttx/can/can.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/** 把控制量限制在归一化输出范围。 */
float CanOutput::constrainUnit(float value)
{
  return value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
}

/** 返回供状态输出使用的协议名称。 */
const char *CanOutput::protocolName(CanProtocol protocol)
{
  return protocol == CanProtocol::Damiao ? "damiao" : "disabled";
}

/** 返回达妙模式的简短名称。 */
const char *CanOutput::modeName(DamiaoMode mode)
{
  if (mode == DamiaoMode::Velocity)
    {
      return "velocity";
    }
  if (mode == DamiaoMode::PositionVelocity)
    {
      return "position_velocity";
    }
  if (mode == DamiaoMode::Mit)
    {
      return "mit";
    }
  return "position_velocity_torque";
}

/** 保存设备路径并绑定执行器、参数和状态 topic。 */
CanOutput::CanOutput(const char *devicePath)
    : _canFd(-1),
      _mixingOutput(*this, MotorCount, ServoCount),
      _parameterSubscription(parameterUpdateTopic()),
      _statusPublication(actuatorStatusTopic()),
      _lastStatusPublish(0),
      _parametersValid(false),
      _protocolArmed(false)
{
  snprintf(_devicePath, sizeof(_devicePath), "%s",
           devicePath == nullptr ? "/dev/can0" : devicePath);
  _status.count = ChannelCount;
}

/** 关闭 CAN 字符设备；任务退出前先发送失能帧。 */
CanOutput::~CanOutput()
{
  if (_canFd >= 0)
    {
      if (__atomic_load_n(&_protocolArmed, __ATOMIC_ACQUIRE))
        {
          (void)sendSpecialAll(0xfd);
        }
      close(_canFd);
      _canFd = -1;
    }
}

/** 创建 CAN 输出独立任务。 */
int CanOutput::task_spawn(int argc, char *argv[])
{
  const int taskId = os::Task::spawn("can_output", 115, 6144,
                                     &run_trampoline, argc, argv);
  if (taskId < 0)
    {
      __atomic_store_n(&_taskId, -1, __ATOMIC_RELEASE);
      return -1;
    }
  __atomic_store_n(&_taskId, taskId, __ATOMIC_RELEASE);
  return wait_until_running();
}

/** 解析可选的 -d 设备路径并创建对象。 */
CanOutput *CanOutput::instantiate(int argc, char *argv[])
{
  const char *path = "/dev/can0";
  for (int index = 0; index < argc; ++index)
    {
      if (strcmp(argv[index], "-d") == 0 && index + 1 < argc)
        {
          path = argv[index + 1];
        }
    }

  CanOutput *output = new CanOutput(path);
  if (output == nullptr || !output->init())
    {
      delete output;
      return nullptr;
    }
  return output;
}

/** 打印模块命令和显式安全门说明。 */
int CanOutput::print_usage(const char *reason)
{
  if (reason != nullptr)
    {
      fprintf(stderr, "%s\n", reason);
    }
  printf("usage: can_output {start [-d /dev/can0]|stop|status|protocols|map|"
         "enable|disable|zero <motor|servo> <index>|"
         "clear <motor|servo> <index>}\n");
  return reason == nullptr ? 0 : -1;
}

/** 打开 NuttX CAN 字符设备并加载参数配置。 */
bool CanOutput::init()
{
  _parametersValid = refreshParameters();
  if (!_parametersValid)
    {
      fprintf(stderr, "invalid CAN output parameters\n");
      return false;
    }

  _canFd = open(_devicePath, O_RDWR | O_NONBLOCK);
  if (_canFd < 0)
    {
      fprintf(stderr, "open %s failed: %d\n", _devicePath, errno);
      return false;
    }
  return true;
}

/** 遍历指定类型的输出并按各通道配置选择协议。 */
bool CanOutput::updateOutputs(ActuatorType type, bool stopMotors,
                              const float *outputs, uint8_t count,
                              uint64_t now)
{
  (void)now;
  os::LockGuard outputGuard(_outputMutex);
  if (!outputGuard.locked()) { return false; }
  // MixingOutput决定是否停止；驱动只把停止状态转换为真实管理帧。
  if (stopMotors)
    {
      if (!__atomic_load_n(&_protocolArmed, __ATOMIC_ACQUIRE) && !_disablePending)
        { return true; }
      const bool stopped = sendSpecialAll(0xfd);
      // 写入失败时保留已使能标志，后续停止回调继续重试。
      if (stopped)
        {
          __atomic_store_n(&_protocolArmed, false, __ATOMIC_RELEASE);
          _disablePending = false;
        }
      else { _disablePending = true; }
      return stopped;
    }
  if (!_parametersValid || _disablePending || outputs == nullptr) { return false; }
  if (!__atomic_load_n(&_protocolArmed, __ATOMIC_ACQUIRE))
    {
      // 即使使能仅部分成功，也必须让停止回调尝试失能所有通道。
      __atomic_store_n(&_protocolArmed, true, __ATOMIC_RELEASE);
      if (!sendSpecialAll(0xfc)) { return false; }
    }

  const uint8_t maximum = type == ActuatorType::Motor ? MotorCount : ServoCount;
  if (count > maximum)
    {
      count = maximum;
    }

  bool success = true;
  for (uint8_t index = 0; index < count; ++index)
    {
      CanChannelConfig config;
      {
        os::LockGuard guard(_configurationMutex);
        if (!guard.locked())
          {
            return false;
          }
        const uint8_t channel = type == ActuatorType::Motor
                                    ? index
                                    : static_cast<uint8_t>(MotorCount + index);
        config = _channels[channel];
      }
      success = sendOne(config, outputs[index]) && success;
    }
  return success;
}

/** 显示设备、安全门和 CAN 收发统计。 */
int CanOutput::print_status()
{
  printf("running\n");
  printf("device: %s\n", _devicePath);
  printf("manual output allowed: %s\n",
         Command::outputInhibited() ? "no" : "yes");
  printf("protocol armed: %s\n",
         __atomic_load_n(&_protocolArmed, __ATOMIC_ACQUIRE) ? "yes" : "no");
  printf("tx/rx/errors: %lu/%lu/%lu\n",
         static_cast<unsigned long>(
             __atomic_load_n(&_status.txCount, __ATOMIC_ACQUIRE)),
         static_cast<unsigned long>(
             __atomic_load_n(&_status.rxCount, __ATOMIC_ACQUIRE)),
         static_cast<unsigned long>(
             __atomic_load_n(&_status.errorCount, __ATOMIC_ACQUIRE)));
  return 0;
}

/** 运行安全门、topic 输出、参数刷新和反馈接收循环。 */
void CanOutput::run()
{
  while (!should_exit())
    {
      const uint64_t now = os::Clock::nowMicroseconds();
      ParameterUpdate update{};
      if (_parameterSubscription.update(update))
        {
          // 必须先停止旧配置，停止发送成功后才能切换ID及协议。
          const bool stopped = updateOutputs(ActuatorType::Motor, true,
                                             nullptr, 0, now);
          if (stopped) { _parametersValid = refreshParameters(); }
          else { _parametersValid = false; _reloadPending = true; }
        }
      if (_reloadPending && updateOutputs(ActuatorType::Motor, true,
                                           nullptr, 0, now))
        {
          _parametersValid = refreshParameters();
          _reloadPending = false;
        }
      (void)_mixingOutput.update();

      receiveFrames(now);
      if (_lastStatusPublish == 0 || now - _lastStatusPublish >= 100000ULL)
        {
          publishStatus(now);
          _lastStatusPublish = now;
        }
      os::Clock::sleepMilliseconds(5);
    }
}

/** 按 M0、M1、S0、S1 参数组生成完整缓存配置。 */
bool CanOutput::refreshParameters()
{
  CanChannelConfig channels[ChannelCount];
  for (uint8_t index = 0; index < MotorCount; ++index)
    {
      if (!loadChannel(ActuatorType::Motor, index, channels[index]))
        {
          return false;
        }
    }
  for (uint8_t index = 0; index < ServoCount; ++index)
    {
      if (!loadChannel(ActuatorType::Servo, index,
                       channels[MotorCount + index]))
        {
          return false;
        }
    }

  os::LockGuard guard(_configurationMutex);
  if (!guard.locked())
    {
      return false;
    }
  memcpy(_channels, channels, sizeof(_channels));
  return true;
}

/** 使用短参数名读取一个执行器，拒绝越界 ID、模式和物理范围。 */
bool CanOutput::loadChannel(ActuatorType type, uint8_t index,
                            CanChannelConfig &config) const
{
  ParamManager &manager = ParamManager::instance();
  const char kind = type == ActuatorType::Motor ? 'M' : 'S';
  char name[ParamNameLength];
  int32_t integer = 0;
  bool valid = true;
  config.actuatorType = type;

#define READ_INT(suffix, target)                                      \
  do                                                                  \
    {                                                                 \
      snprintf(name, sizeof(name), "CAN_%c%u_" suffix, kind, index); \
      valid = manager.get(name, integer) && valid;                    \
      target = integer;                                               \
    }                                                                 \
  while (0)
#define READ_FLOAT(suffix, target)                                    \
  do                                                                  \
    {                                                                 \
      snprintf(name, sizeof(name), "CAN_%c%u_" suffix, kind, index); \
      valid = manager.get(name, target) && valid;                     \
    }                                                                 \
  while (0)

  READ_INT("PROTO", integer);
  config.protocol = static_cast<CanProtocol>(integer);
  READ_INT("TYPE", integer);
  config.motorType = static_cast<DamiaoMotorType>(integer);
  READ_INT("ID", integer);
  config.motorId = static_cast<uint16_t>(integer);
  READ_INT("FBID", integer);
  config.feedbackId = static_cast<uint16_t>(integer);
  READ_INT("MODE", integer);
  config.mode = static_cast<DamiaoMode>(integer);
  READ_FLOAT("REV", config.reverse);
  READ_FLOAT("VMAX", config.limits.velocityMax);
  READ_FLOAT("TMAX", config.limits.torqueMax);
  if (type == ActuatorType::Servo)
    {
      READ_FLOAT("PMIN", config.positionMinimum);
      READ_FLOAT("PMAX", config.positionMaximum);
      READ_FLOAT("PFBMAX", config.limits.positionMax);
    }
  else
    {
      READ_FLOAT("PMAX", config.limits.positionMax);
      config.positionMinimum = -config.limits.positionMax;
      config.positionMaximum = config.limits.positionMax;
    }

#undef READ_INT
#undef READ_FLOAT

  uint16_t offset = 0;
  valid = valid &&
          (config.protocol == CanProtocol::Disabled ||
           config.protocol == CanProtocol::Damiao) &&
          DamiaoProtocol::modeOffset(config.mode, offset) &&
          config.motorId + offset <= 0x7ffU && config.feedbackId <= 0x7ffU &&
          (config.reverse == 1.0f || config.reverse == -1.0f) &&
          config.limits.positionMax > 0.0f &&
          config.limits.velocityMax > 0.0f &&
          config.limits.torqueMax > 0.0f &&
          config.positionMinimum < config.positionMaximum;
  return valid;
}

/** 当前只注册达妙协议，Disabled 通道不发送帧。 */
bool CanOutput::sendOne(const CanChannelConfig &config, float output)
{
  if (config.protocol == CanProtocol::Disabled)
    {
      return true;
    }
  if (config.protocol == CanProtocol::Damiao)
    {
      return sendDamiao(config, output);
    }
  return false;
}

/** 动力电机使用速度模式，转向执行器使用位置速度模式。 */
bool CanOutput::sendDamiao(const CanChannelConfig &config, float output)
{
  output = constrainUnit(output) * config.reverse;
  CanFrame frame;
  bool encoded = false;
  if (config.actuatorType == ActuatorType::Motor &&
      config.mode == DamiaoMode::Velocity)
    {
      encoded = DamiaoProtocol::encodeVelocity(
          config.motorId, output * config.limits.velocityMax, frame);
    }
  else if (config.actuatorType == ActuatorType::Servo &&
           config.mode == DamiaoMode::PositionVelocity)
    {
      const float ratio = (output + 1.0f) * 0.5f;
      const float position = config.positionMinimum +
                             ratio * (config.positionMaximum -
                                      config.positionMinimum);
      encoded = DamiaoProtocol::encodePositionVelocity(
          config.motorId, position, config.limits.velocityMax, frame);
    }

  if (!encoded)
    {
      __atomic_add_fetch(&_status.errorCount, 1U, __ATOMIC_RELAXED);
      return false;
    }
  return writeFrame(frame);
}

/** 将项目 CanFrame 转为 NuttX can_msg_s 并进行一次完整写入。 */
bool CanOutput::writeFrame(const CanFrame &frame)
{
  if (_canFd < 0 || frame.length > 8 || frame.id > 0x7ffU)
    {
      return false;
    }

  struct can_msg_s message{};
  message.cm_hdr.ch_id = frame.id;
  message.cm_hdr.ch_dlc = can_bytes2dlc(frame.length);
  message.cm_hdr.ch_rtr = 0;
  message.cm_hdr.ch_tcf = 0;
  memcpy(message.cm_data, frame.data, frame.length);
  const size_t length = CAN_MSGLEN(frame.length);

  os::LockGuard guard(_writeMutex);
  const ssize_t written = guard.locked()
                              ? write(_canFd, &message, length)
                              : static_cast<ssize_t>(-1);
  if (written != static_cast<ssize_t>(length))
    {
      __atomic_add_fetch(&_status.errorCount, 1U, __ATOMIC_RELAXED);
      return false;
    }
  __atomic_add_fetch(&_status.txCount, 1U, __ATOMIC_RELAXED);
  return true;
}

/** 清空本周期已经到达的非阻塞 CAN 消息。 */
void CanOutput::receiveFrames(uint64_t now)
{
  for (uint8_t receivedCount = 0; receivedCount < 32 && _canFd >= 0; ++receivedCount)
    {
      struct can_msg_s message{};
      const ssize_t received = read(_canFd, &message, sizeof(message));
      if (received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
          return;
        }
      if (received < static_cast<ssize_t>(CAN_MSGLEN(0)))
        {
          if (received != 0)
            {
              __atomic_add_fetch(&_status.errorCount, 1U,
                                 __ATOMIC_RELAXED);
            }
          return;
        }

      CanFrame frame;
      frame.id = message.cm_hdr.ch_id;
      frame.length = can_dlc2bytes(message.cm_hdr.ch_dlc);
      if (frame.length > 8)
        {
          __atomic_add_fetch(&_status.errorCount, 1U, __ATOMIC_RELAXED);
          continue;
        }
      memcpy(frame.data, message.cm_data, frame.length);
      (void)handleFeedback(frame, now);
    }
}

/** 以反馈 CAN ID 和数据中的电机 ID 双重匹配一个已配置通道。 */
bool CanOutput::handleFeedback(const CanFrame &frame, uint64_t now)
{
  for (uint8_t index = 0; index < ChannelCount; ++index)
    {
      CanChannelConfig config;
      {
        os::LockGuard guard(_configurationMutex);
        if (!guard.locked())
          {
            return false;
          }
        config = _channels[index];
      }
      if (config.protocol != CanProtocol::Damiao ||
          (config.feedbackId != 0 && config.feedbackId != frame.id))
        {
          continue;
        }

      DamiaoFeedback feedback;
      if (!DamiaoProtocol::decodeFeedback(frame, config.limits, feedback) ||
          feedback.motorId != (config.motorId & 0x0fU))
        {
          continue;
        }

      _status.timestampSample[index] = now;
      _status.position[index] = feedback.position;
      _status.velocity[index] = feedback.velocity;
      _status.torque[index] = feedback.torque;
      _status.mosTemperature[index] = feedback.mosTemperature;
      _status.coilTemperature[index] = feedback.coilTemperature;
      _status.error[index] = feedback.error;
      _status.online[index] = true;
      __atomic_add_fetch(&_status.rxCount, 1U, __ATOMIC_RELAXED);
      return true;
    }
  return false;
}

/** 顺序向四个通道发送达妙管理帧。 */
bool CanOutput::sendSpecialAll(uint8_t command)
{
  bool success = true;
  for (uint8_t index = 0; index < MotorCount; ++index)
    {
      success = sendSpecial(ActuatorType::Motor, index, command) && success;
      // NuttX CAN 设备以非阻塞方式打开。管理帧之间留出队列排空时间，
      // 防止连续四帧时排在最后的后转向电机收不到使能命令。
      os::Clock::sleepMilliseconds(2);
    }
  for (uint8_t index = 0; index < ServoCount; ++index)
    {
      success = sendSpecial(ActuatorType::Servo, index, command) && success;
      os::Clock::sleepMilliseconds(2);
    }
  return success;
}

/** 根据通道模式构造使能、失能、设零或清错帧。 */
bool CanOutput::sendSpecial(ActuatorType type, uint8_t index, uint8_t command)
{
  const uint8_t channel = type == ActuatorType::Motor
                              ? index
                              : static_cast<uint8_t>(MotorCount + index);
  if (channel >= ChannelCount)
    {
      return false;
    }

  CanChannelConfig config;
  {
    os::LockGuard guard(_configurationMutex);
    if (!guard.locked())
      {
        return false;
      }
    config = _channels[channel];
  }
  if (config.protocol == CanProtocol::Disabled)
    {
      return true;
    }
  if (config.protocol != CanProtocol::Damiao)
    {
      return false;
    }

  CanFrame frame;
  bool encoded = false;
  if (command == 0xfc)
    encoded = DamiaoProtocol::encodeEnable(config.motorId, config.mode, frame);
  else if (command == 0xfd)
    encoded = DamiaoProtocol::encodeDisable(config.motorId, config.mode, frame);
  else if (command == 0xfe)
    encoded = DamiaoProtocol::encodeSetZero(config.motorId, config.mode, frame);
  else if (command == 0xfb)
    encoded = DamiaoProtocol::encodeClearError(config.motorId, config.mode, frame);
  return encoded && writeFrame(frame);
}

/** 打印四个输出槽位当前缓存的协议、ID 和模式。 */
void CanOutput::printMap()
{
  os::LockGuard guard(_configurationMutex);
  if (!guard.locked())
    {
      return;
    }
  for (uint8_t index = 0; index < ChannelCount; ++index)
    {
      const CanChannelConfig &config = _channels[index];
      const bool motor = index < MotorCount;
      const uint8_t localIndex = motor ? index : index - MotorCount;
      printf("%s %u: %s id=%u fbid=%u mode=%s\n",
             motor ? "motor" : "servo", localIndex,
             protocolName(config.protocol), config.motorId,
             config.feedbackId, modeName(config.mode));
      if (!motor)
        {
          printf("  command position: %.3f..%.3f rad, "
                 "feedback position: +/-%0.3f rad, vmax: %.3f rad/s\n",
                 static_cast<double>(config.positionMinimum),
                 static_cast<double>(config.positionMaximum),
                 static_cast<double>(config.limits.positionMax),
                 static_cast<double>(config.limits.velocityMax));
        }
    }
}

/** 每 100 ms 发布一次反馈；超过 500 ms 没有新帧标记为离线。 */
void CanOutput::publishStatus(uint64_t now)
{
  _status.timestamp = now;
  for (uint8_t index = 0; index < ChannelCount; ++index)
    {
      _status.online[index] = _status.timestampSample[index] != 0 &&
                              now >= _status.timestampSample[index] &&
                              now - _status.timestampSample[index] <= 500000ULL;
    }
  (void)_statusPublication.publish(_status);
}

/** 提供NSH的can_output命令入口，转交ModuleBase统一处理生命周期。 */
extern "C" int can_output_main(int argc, char *argv[])
{
  return CanOutput::main(argc, argv);
}
