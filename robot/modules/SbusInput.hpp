/**
 * @file SbusInput.hpp
 * @brief 声明 USART3 SBUS 接收与 input_rc 发布模块。
 */

#pragma once

#include "robot/common/ModuleBase.hpp"
#include "robot/drivers/rc/sbus.h"
#include "robot/orb/Topics.hpp"

#include <stdint.h>

/**
 * @class SbusInput
 * @brief 从 MK32 接收机读取 SBUS 字节，解析后发布 input_rc。
 */
class SbusInput final : public ModuleBase<SbusInput>
{
public:
  /** 保存串口设备路径并初始化驱动状态。 */
  explicit SbusInput(const char *devicePath);

  /** 关闭已经打开的 SBUS 串口。 */
  ~SbusInput() override;

  /** 创建 sbus_input 独立线程。 */
  static int task_spawn(int argc, char *argv[]);

  /** 解析 -d 参数、创建对象并打开 SBUS 串口。 */
  static SbusInput *instantiate(int argc, char *argv[]);

  /** 返回统一命令路由使用的固定模块名称。 */
  static const char *command_name() { return "sbus_input"; }

  /** 打印 sbus_input 命令帮助。 */
  static int print_usage(const char *reason = nullptr);

  /** 打印设备、收帧统计、原始主通道和链路状态。 */
  int print_status() override;

  /** 循环读取串口、解析 SBUS 并发布 input_rc。 */
  void run() override;

private:
  /** 从启动参数读取串口设备路径，默认使用ttyS2。 */
  static const char *parseDevicePath(int argc, char *argv[]);
  /** 唯一扩展命令路由可以访问受保护的模块实例。 */
  friend class CommandRouter;
  /** 按 100000 波特率、8E2、非阻塞方式打开串口。 */
  bool initialize();

  /** 将一帧 SBUS 数据转换为 InputRc 并发布。 */
  void publishFrame(const SbusFrame &frame, uint64_t timestamp);

  /** 在超过超时时间没有收到数据时发布一次 RC lost。 */
  void publishLossIfNeeded(uint64_t timestamp);

  static constexpr uint64_t SignalTimeoutMicroseconds = 500000;

  char _devicePath[32];
  int _fileDescriptor;
  SbusDecoder _decoder;
  uorb::Publication<InputRc> _publication;
  InputRc _lastMessage;
  uint64_t _lastSignalTimestamp;
  uint32_t _publishedFrames;
  uint32_t _readErrors;
  bool _lossPublished;
};
