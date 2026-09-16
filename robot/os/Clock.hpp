/**
 * @file Clock.hpp
 * @brief 声明单调时钟读取和线程休眠的操作系统接口。
 */

#pragma once

#include <stdint.h>

namespace os
{

/** @class Clock @brief 为业务模块屏蔽 NuttX/POSIX 时间接口。 */
class Clock
{
public:
  /** 返回系统启动后的单调时间，单位为微秒。 */
  static uint64_t nowMicroseconds();

  /** 让当前线程休眠指定的毫秒数。 */
  static void sleepMilliseconds(uint32_t milliseconds);
};

} // namespace os
