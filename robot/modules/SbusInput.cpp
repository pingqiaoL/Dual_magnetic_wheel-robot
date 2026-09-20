/**
 * @file SbusInput.cpp
 * @brief 实现 MK32 SBUS 串口配置、接收循环、链路超时和 uORB 发布。
 */

// NuttX Make对命令源文件定义main宏；显式命名入口无需重命名，
// 取消该宏以避免改写ModuleBase::main等C++成员名称。
#ifdef main
#undef main
#endif

#include "robot/modules/SbusInput.hpp"

#include "robot/os/Clock.hpp"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>


/** 从 start 后的参数中读取 -d 串口设备选项。 */
const char *SbusInput::parseDevicePath(int argc, char *argv[])
{
  const char *devicePath = "/dev/ttyS2";
  for (int index = 0; index < argc; ++index)
    {
      if (strcmp(argv[index], "-d") == 0 && index + 1 < argc)
        {
          devicePath = argv[index + 1];
          ++index;
        }
    }

  return devicePath;
}

/** 初始化设备路径、uORB 发布器和运行统计。 */
SbusInput::SbusInput(const char *devicePath)
    : _fileDescriptor(-1),
      _publication(inputRcTopic()),
      _lastSignalTimestamp(0),
      _publishedFrames(0),
      _readErrors(0),
      _lossPublished(false)
{
  snprintf(_devicePath, sizeof(_devicePath), "%s",
           devicePath != nullptr ? devicePath : "/dev/ttyS2");
}

/** 释放串口文件描述符。 */
SbusInput::~SbusInput()
{
  if (_fileDescriptor >= 0)
    {
      close(_fileDescriptor);
      _fileDescriptor = -1;
    }
}

/** 标记模块正在启动并创建 SBUS 接收线程。 */
int SbusInput::task_spawn(int argc, char *argv[])
{
  const int taskId = os::Task::spawn("sbus_input", 120, 4096,
                                     &run_trampoline, argc, argv);
  if (taskId < 0)
    {
      __atomic_store_n(&_taskId, -1, __ATOMIC_RELEASE);
      return -1;
    }

  __atomic_store_n(&_taskId, taskId, __ATOMIC_RELEASE);
  return wait_until_running();
}

/** 创建对象并验证 USART3 对应的字符设备可以打开。 */
SbusInput *SbusInput::instantiate(int argc, char *argv[])
{
  SbusInput *instance = new SbusInput(parseDevicePath(argc, argv));
  if (instance == nullptr)
    {
      return nullptr;
    }

  if (!instance->initialize())
    {
      delete instance;
      return nullptr;
    }

  return instance;
}

/** 打印 SBUS 模块命令及默认设备。 */
int SbusInput::print_usage(const char *reason)
{
  if (reason != nullptr)
    {
      fprintf(stderr, "%s\n", reason);
    }

  printf("usage: sbus_input {start [-d device]|stop|status}\n");
  printf("default device: %s (USART3 RX PC11, 100000 8E2)\n",
         "/dev/ttyS2");
  return reason == nullptr ? 0 : -1;
}

/** 输出串口路径、帧统计、原始主通道和当前链路标志。 */
int SbusInput::print_status()
{
  printf("running\n");
  printf("device: %s\n", _devicePath);
  printf("frames: %lu\n",
         static_cast<unsigned long>(
             __atomic_load_n(&_publishedFrames, __ATOMIC_ACQUIRE)));
  printf("decoder errors: %lu\n",
         static_cast<unsigned long>(_decoder.invalidFrames()));
  printf("receiver lost frames: %lu\n",
         static_cast<unsigned long>(_decoder.receiverLostFrames()));
  printf("read errors: %lu\n",
         static_cast<unsigned long>(
             __atomic_load_n(&_readErrors, __ATOMIC_ACQUIRE)));
  printf("rc lost: %s\n",
         __atomic_load_n(&_lossPublished, __ATOMIC_ACQUIRE) ? "yes" : "no");

  InputRc input;
  if (inputRcTopic().copy(input))
    {
      printf("channels: %u\n", static_cast<unsigned>(input.channelCount));
      printf("raw ch1-4: %u %u %u %u\n",
             static_cast<unsigned>(input.values[0]),
             static_cast<unsigned>(input.values[1]),
             static_cast<unsigned>(input.values[2]),
             static_cast<unsigned>(input.values[3]));
      printf("frame lost: %s, failsafe: %s\n",
             input.frameLost ? "yes" : "no",
             input.rcFailsafe ? "yes" : "no");
    }
  else
    {
      printf("latest input_rc: unavailable\n");
    }

  return 0;
}

/** 使用 NuttX termios 配置标准 SBUS 串口参数。 */
bool SbusInput::initialize()
{
  _fileDescriptor = open(_devicePath, O_RDONLY | O_NONBLOCK);
  if (_fileDescriptor < 0)
    {
      fprintf(stderr, "failed to open SBUS device %s: %d\n",
              _devicePath, errno);
      return false;
    }

  struct termios options;
  if (tcgetattr(_fileDescriptor, &options) != 0)
    {
      fprintf(stderr, "failed to read SBUS termios: %d\n", errno);
      return false;
    }

  options.c_iflag = INPCK | IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;
  options.c_cflag &= ~(CSIZE | PARODD);
#ifdef CRTSCTS
  options.c_cflag &= ~CRTSCTS;
#endif
  options.c_cflag |= CS8 | CSTOPB | PARENB | CLOCAL | CREAD;
  options.c_cc[VMIN] = 0;
  options.c_cc[VTIME] = 0;

  if (cfsetspeed(&options, 100000) != 0 ||
      tcsetattr(_fileDescriptor, TCSANOW, &options) != 0)
    {
      fprintf(stderr, "failed to configure SBUS termios: %d\n", errno);
      return false;
    }

  tcflush(_fileDescriptor, TCIFLUSH);
  _lastSignalTimestamp = os::Clock::nowMicroseconds();
  return true;
}

/** 非阻塞读取 USART3 并逐字节送入协议解析器。 */
void SbusInput::run()
{
  uint8_t buffer[64];

  while (!should_exit())
    {
      const ssize_t bytesRead = read(_fileDescriptor, buffer, sizeof(buffer));
      const uint64_t now = os::Clock::nowMicroseconds();

      if (bytesRead > 0)
        {
          for (ssize_t index = 0; index < bytesRead; ++index)
            {
              SbusFrame frame;
              if (_decoder.feed(buffer[index], now, frame))
                {
                  publishFrame(frame, now);
                }
            }
        }
      else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK &&
               errno != EINTR)
        {
          __atomic_add_fetch(&_readErrors, 1U, __ATOMIC_RELAXED);
        }

      publishLossIfNeeded(now);
      os::Clock::sleepMilliseconds(2);
    }
}

/** 生成 input_rc 消息并通过 uORB 发布。 */
void SbusInput::publishFrame(const SbusFrame &frame, uint64_t timestamp)
{
  InputRc message;
  message.timestamp = timestamp;
  message.timestampLastSignal = timestamp;
  message.channelCount = frame.channelCount;
  message.inputSource = InputRc::SOURCE_SBUS;
  message.rssi = -1;
  message.rcFailsafe = frame.failsafe;
  message.rcLost = false;
  message.frameLost = frame.frameLost;
  message.lostFrameCount = _decoder.receiverLostFrames();
  message.totalFrameCount = _decoder.totalFrames();

  for (uint8_t channel = 0; channel < frame.channelCount; ++channel)
    {
      message.values[channel] = frame.values[channel];
    }

  _lastMessage = message;
  _lastSignalTimestamp = timestamp;
  __atomic_store_n(&_lossPublished, false, __ATOMIC_RELEASE);
  if (_publication.publish(message))
    {
      __atomic_add_fetch(&_publishedFrames, 1U, __ATOMIC_RELAXED);
    }
}

/** 超过 500 ms 没有完整帧时发布一次 rcLost 状态。 */
void SbusInput::publishLossIfNeeded(uint64_t timestamp)
{
  if (__atomic_load_n(&_lossPublished, __ATOMIC_ACQUIRE) ||
      timestamp - _lastSignalTimestamp <= SignalTimeoutMicroseconds)
    {
      return;
    }

  _lastMessage.timestamp = timestamp;
  _lastMessage.rcLost = true;
  _lastMessage.frameLost = false;
  _lastMessage.lostFrameCount = _decoder.receiverLostFrames();
  _lastMessage.totalFrameCount = _decoder.totalFrames();
  _publication.publish(_lastMessage);
  __atomic_store_n(&_lossPublished, true, __ATOMIC_RELEASE);
}

/** 提供NSH的sbus_input命令入口，转交ModuleBase统一处理生命周期。 */
extern "C" int sbus_input_main(int argc, char *argv[])
{
  return SbusInput::main(argc, argv);
}
