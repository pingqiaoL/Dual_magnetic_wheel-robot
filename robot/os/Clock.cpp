/**
 * @file Clock.cpp
 * @brief 使用 clock_gettime 和 nanosleep 实现系统时间接口。
 */

#include "robot/os/Clock.hpp"

#include <errno.h>
#include <time.h>

namespace os
{

/** 读取不会受系统时间校准影响的单调时钟。 */
uint64_t Clock::nowMicroseconds()
{
  struct timespec timeValue;
  if (clock_gettime(CLOCK_MONOTONIC, &timeValue) != 0)
    {
      return 0;
    }

  return static_cast<uint64_t>(timeValue.tv_sec) * 1000000ULL +
         static_cast<uint64_t>(timeValue.tv_nsec) / 1000ULL;
}

/** 执行可被信号中断并自动继续的毫秒级休眠。 */
void Clock::sleepMilliseconds(uint32_t milliseconds)
{
  struct timespec request;
  request.tv_sec = milliseconds / 1000;
  request.tv_nsec = static_cast<long>(milliseconds % 1000) * 1000000L;

  while (nanosleep(&request, &request) != 0 && errno == EINTR)
    {
    }
}

} // namespace os
