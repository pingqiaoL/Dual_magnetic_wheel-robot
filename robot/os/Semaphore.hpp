/**
 * @file Semaphore.hpp
 * @brief 声明用于线程同步和限时等待的信号量封装。
 */

#pragma once

#include <semaphore.h>
#include <stdint.h>

namespace os
{

/** @class Semaphore @brief 封装 POSIX 信号量，供驱动和任务之间同步事件。 */
class Semaphore
{
public:
  /** 使用给定初值创建进程内信号量。 */
  explicit Semaphore(unsigned int initialValue = 0);

  /** 销毁信号量。 */
  ~Semaphore();

  /** 禁止复制底层信号量。 */
  Semaphore(const Semaphore &) = delete;

  /** 禁止通过赋值共享底层信号量。 */
  Semaphore &operator=(const Semaphore &) = delete;

  /** 增加一次信号量计数并唤醒等待线程。 */
  bool post();

  /** 一直等待到获得信号量。 */
  bool wait();

  /** 在指定毫秒数内等待信号量。 */
  bool waitFor(uint32_t timeoutMilliseconds);

private:
  sem_t _semaphore;
};

} // namespace os
