/**
 * @file Mutex.cpp
 * @brief 使用 pthread_mutex_t 实现互斥锁和作用域锁。
 */

#include "robot/os/Mutex.hpp"

namespace os
{

/** 初始化底层 pthread 互斥锁。 */
Mutex::Mutex()
{
  pthread_mutex_init(&_mutex, nullptr);
}

/** 销毁底层 pthread 互斥锁。 */
Mutex::~Mutex()
{
  pthread_mutex_destroy(&_mutex);
}

/** 阻塞等待并获取互斥锁。 */
bool Mutex::lock()
{
  return pthread_mutex_lock(&_mutex) == 0;
}

/** 释放互斥锁。 */
void Mutex::unlock()
{
  pthread_mutex_unlock(&_mutex);
}

/** 在构造时取得指定互斥锁。 */
LockGuard::LockGuard(Mutex &mutex) : _mutex(mutex), _locked(mutex.lock())
{
}

/** 离开作用域时自动释放已经取得的锁。 */
LockGuard::~LockGuard()
{
  if (_locked)
    {
      _mutex.unlock();
    }
}

/** 查询作用域锁是否成功取得底层互斥锁。 */
bool LockGuard::locked() const
{
  return _locked;
}

} // namespace os
