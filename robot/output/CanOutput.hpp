/**
 * @file CanOutput.hpp
 * @brief 声明使用 NuttX 字符设备 /dev/can0 的参数化 CAN 输出模块。
 */

#pragma once

#include "protocol/can/CanProtocol.hpp"
#include "protocol/can/DamiaoProtocol.hpp"
#include "robot/common/ModuleBase.hpp"
#include "robot/orb/Topics.hpp"
#include "robot/os/Mutex.hpp"
#include "robot/output/OutputInterface.hpp"
#include "robot/output/MixingOutput.hpp"

#include <stdint.h>

/** 一个执行器通道从参数系统缓存的完整 CAN 配置。 */
struct CanChannelConfig
{
  ActuatorType actuatorType{ActuatorType::Motor};
  CanProtocol protocol{CanProtocol::Disabled};
  DamiaoMotorType motorType{DamiaoMotorType::Unknown};
  DamiaoMode mode{DamiaoMode::Velocity};
  uint16_t motorId{0};
  uint16_t feedbackId{0};
  float reverse{1.0f};
  float positionMinimum{-1.0f};       // 机械指令下限，rad
  float positionMaximum{1.0f};        // 机械指令上限，rad
  DamiaoLimits limits{};              // 达妙协议反馈量程
};

/** 订阅执行器 topic、选择协议并通过 NuttX CAN 驱动收发帧。 */
class CanOutput final : public ModuleBase<CanOutput>, public OutputInterface
{
public:
  explicit CanOutput(const char *devicePath);
  ~CanOutput() override;

  static int task_spawn(int argc, char *argv[]);
  static CanOutput *instantiate(int argc, char *argv[]);
  /** 返回统一命令路由使用的固定模块名称。 */
  static const char *command_name() { return "can_output"; }
  static int print_usage(const char *reason = nullptr);

  bool init() override;
  bool updateOutputs(ActuatorType type, bool stopMotors,
                     const float *outputs, uint8_t count,
                     uint64_t now) override;
  int print_status() override;
  void run() override;

private:
  /** 唯一扩展命令路由可以访问受保护的模块实例。 */
  friend class CommandRouter;
  /** 协议编码前再次保护归一化数值范围。 */
  static float constrainUnit(float value);
  /** 将协议枚举转换为NSH显示名称。 */
  static const char *protocolName(CanProtocol protocol);
  /** 将达妙模式枚举转换为NSH显示名称。 */
  static const char *modeName(DamiaoMode mode);
  static constexpr uint8_t MotorCount = 2;
  static constexpr uint8_t ServoCount = 2;
  static constexpr uint8_t ChannelCount = MotorCount + ServoCount;

  /** 从参数表构造新配置，通过校验后一次替换缓存。 */
  bool refreshParameters();

  /** 读取一个 M/S 通道的全部参数。 */
  bool loadChannel(ActuatorType type, uint8_t index,
                   CanChannelConfig &config) const;

  /** 根据缓存协议把一个归一化输出编码并发送。 */
  bool sendOne(const CanChannelConfig &config, float output);

  /** 调用达妙速度或位置速度模式编码器。 */
  bool sendDamiao(const CanChannelConfig &config, float output);

  /** 通过 NuttX write 一次发送完整标准 CAN 帧。 */
  bool writeFrame(const CanFrame &frame);

  /** 非阻塞读取 CAN FIFO，并发布能识别的达妙反馈。 */
  void receiveFrames(uint64_t now);

  /** 把一帧达妙反馈匹配到对应执行器通道。 */
  bool handleFeedback(const CanFrame &frame, uint64_t now);

  /** 向所有启用的达妙通道发送使能、失能等管理帧。 */
  bool sendSpecialAll(uint8_t command);

  /** 向指定类型和编号的通道发送单个管理帧。 */
  bool sendSpecial(ActuatorType type, uint8_t index, uint8_t command);

  /** 输出 NSH map 命令需要的当前配置。 */
  void printMap();

  /** 更新 online 标志并发布 actuator_status。 */
  void publishStatus(uint64_t now);

  char _devicePath[32];
  int _canFd;
  CanChannelConfig _channels[ChannelCount];
  os::Mutex _configurationMutex;
  os::Mutex _writeMutex;
  /** 串行化解锁/失锁及NSH设零操作，避免管理帧之间的竞争。 */
  os::Mutex _outputMutex;
  /** 拥有公共处理器，构造时传入本驱动的*this供虚函数回调。 */
  MixingOutput _mixingOutput;
  uorb::Subscription<ParameterUpdate> _parameterSubscription;
  uorb::Publication<ActuatorStatus> _statusPublication;
  ActuatorStatus _status;
  uint64_t _lastStatusPublish;
  bool _parametersValid;
  bool _reloadPending{false};
  /** 初次运行也必须下发失能，不能假定电机随MCU复位而失能。 */
  bool _disablePending{true};
  bool _protocolArmed;
};
