/**
 * @file Semaphore.cpp
 * @brief 使用 POSIX sem 接口实现永久等待和限时等待。
 */

#include "robot/os/Semaphore.hpp"

#include <errno.h>
#include <time.h>

namespace os
{

/** 初始化进程内信号量。 */
Semaphore::Semaphore(unsigned int initialValue)
{
  sem_init(&_semaphore, 0, initialValue);
}

/** 释放信号量资源。 */
Semaphore::~Semaphore()
{
  sem_destroy(&_semaphore);
}

/** 发布一次同步事件。 */
bool Semaphore::post()
{
  return sem_post(&_semaphore) == 0;
}

/** 等待同步事件，并在信号中断后继续等待。 */
bool Semaphore::wait()
{
  int result;
  do
    {
      result = sem_wait(&_semaphore);
    }
  while (result < 0 && errno == EINTR);

  return result == 0;
}

/** 使用绝对截止时间等待同步事件。 */
bool Semaphore::waitFor(uint32_t timeoutMilliseconds)
{
  struct timespec deadline;
  if (clock_gettime(CLOCK_REALTIME, &deadline) != 0)
    {
      return false;
    }

  deadline.tv_sec += timeoutMilliseconds / 1000;
  deadline.tv_nsec += static_cast<long>(timeoutMilliseconds % 1000) * 1000000L;
  if (deadline.tv_nsec >= 1000000000L)
    {
      deadline.tv_sec += 1;
      deadline.tv_nsec -= 1000000000L;
    }

  int result;
  do
    {
      result = sem_timedwait(&_semaphore, &deadline);
    }
  while (result < 0 && errno == EINTR);

  return result == 0;
}

} // namespace os
