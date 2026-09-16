/**
 * @file uORB.hpp
 * @brief 提供适合当前单片机工程的轻量发布、订阅和最新值缓存机制。
 *
 * 当前实现不使用任务队列，也不在中断中调用。每个 Topic 保存最新一份
 * 消息和递增的 generation；Publication 发布消息，Subscription 只在发现
 * generation 变化时取出新消息，从而让驱动和业务模块不直接互相调用。
 */

#pragma once

#include "robot/os/Mutex.hpp"

#include <stdint.h>

namespace uorb
{

/**
 * @class Topic
 * @brief 保存某一种消息的最新数据，并使用互斥锁保护发布和复制。
 */
template<typename Message>
class Topic
{
public:
  /** 创建尚未发布过数据的 topic。 */
  Topic() : _generation(0), _advertised(false)
  {
  }

  /** 禁止复制带有互斥锁和 generation 的 topic。 */
  Topic(const Topic &) = delete;

  /** 禁止通过赋值复制 topic 状态。 */
  Topic &operator=(const Topic &) = delete;

  /** 发布一份消息，并增加 generation 通知所有订阅者。 */
  bool publish(const Message &message)
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked())
      {
        return false;
      }

    _message = message;
    ++_generation;
    _advertised = true;
    return true;
  }

  /** 当存在新 generation 时复制消息，并更新订阅者自己的 generation。 */
  bool update(uint32_t &subscriberGeneration, Message &message)
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked() || !_advertised ||
        subscriberGeneration == _generation)
      {
        return false;
      }

    message = _message;
    subscriberGeneration = _generation;
    return true;
  }

  /** 无论 generation 是否变化都复制最新消息。 */
  bool copy(Message &message)
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked() || !_advertised)
      {
        return false;
      }

    message = _message;
    return true;
  }

private:
  /** 保护消息、generation 和 advertised 状态。 */
  os::Mutex _mutex;

  /** topic 中保存的最新一份消息。 */
  Message _message{};

  /** 每发布一次加一，用于区分订阅者是否已读取。 */
  uint32_t _generation;

  /** 标记 topic 是否至少成功发布过一次。 */
  bool _advertised;
};

/**
 * @class Publication
 * @brief 持有一个 topic，并向它发布指定类型的消息。
 */
template<typename Message>
class Publication
{
public:
  /** 绑定需要发布的 topic。 */
  explicit Publication(Topic<Message> &topic) : _topic(topic)
  {
  }

  /** 将消息写入绑定的 topic。 */
  bool publish(const Message &message)
  {
    return _topic.publish(message);
  }

private:
  /** 发布目标。 */
  Topic<Message> &_topic;
};

/**
 * @class Subscription
 * @brief 保存自己的读取 generation，并只取出尚未处理的新消息。
 */
template<typename Message>
class Subscription
{
public:
  /** 绑定需要订阅的 topic，并从 generation 0 开始读取。 */
  explicit Subscription(Topic<Message> &topic)
      : _topic(topic), _generation(0)
  {
  }

  /** 存在新消息时复制并返回 true。 */
  bool update(Message &message)
  {
    return _topic.update(_generation, message);
  }

  /** 复制 topic 的最新消息，不改变订阅 generation。 */
  bool copy(Message &message)
  {
    return _topic.copy(message);
  }

private:
  /** 订阅来源。 */
  Topic<Message> &_topic;

  /** 此订阅者最后读取的 generation。 */
  uint32_t _generation;
};

} // namespace uorb
