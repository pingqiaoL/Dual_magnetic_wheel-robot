/**
 * @file Mutex.hpp
 * @brief 声明互斥锁及其自动加解锁辅助类。
 */

#pragma once

#include <pthread.h>

namespace os
{

/** @class Mutex @brief 封装 pthread_mutex_t，保护多个线程共享的数据。 */
class Mutex
{
public:
  /** 初始化 pthread 互斥锁。 */
  Mutex();

  /** 销毁 pthread 互斥锁。 */
  ~Mutex();

  /** 禁止复制底层互斥锁。 */
  Mutex(const Mutex &) = delete;

  /** 禁止通过赋值共享底层互斥锁。 */
  Mutex &operator=(const Mutex &) = delete;

  /** 获取互斥锁，成功返回 true。 */
  bool lock();

  /** 释放互斥锁。 */
  void unlock();

private:
  pthread_mutex_t _mutex;
};

/** @class LockGuard @brief 在作用域开始时加锁并在离开作用域时自动解锁。 */
class LockGuard
{
public:
  /** 尝试锁住传入的 Mutex。 */
  explicit LockGuard(Mutex &mutex);

  /** 如果构造时成功加锁，则自动解锁。 */
  ~LockGuard();

  /** 禁止复制作用域锁，避免重复解锁。 */
  LockGuard(const LockGuard &) = delete;

  /** 禁止给作用域锁赋值，避免锁所有权不清。 */
  LockGuard &operator=(const LockGuard &) = delete;

  /** 返回构造时是否成功取得互斥锁。 */
  bool locked() const;

private:
  Mutex &_mutex;
  bool _locked;
};

} // namespace os
