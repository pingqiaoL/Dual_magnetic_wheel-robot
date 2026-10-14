#pragma once
#include "robot/orb/uORB.hpp"
#include "robot/work_queue/WorkItem.hpp"
namespace uorb
{
/** Callback only queues work; it never executes module logic in a publisher. */
template<typename Message>
class SubscriptionCallbackWorkItem : public Subscription<Message>, public Callback
{
public:
  SubscriptionCallbackWorkItem(Topic<Message> &topic, robot_wq::WorkItem &work)
    : Subscription<Message>(topic), _topic(topic), _work(work) {}
  ~SubscriptionCallbackWorkItem() override { unregisterCallback(); }
  SubscriptionCallbackWorkItem(const SubscriptionCallbackWorkItem &) = delete;
  SubscriptionCallbackWorkItem &operator=(const SubscriptionCallbackWorkItem &) = delete;
  bool registerCallback()
  {
    if (_registered) { return true; }
    _registered = _topic.registerCallback(*this);
    return _registered;
  }
  void unregisterCallback()
  {
    if (_registered) { _topic.unregisterCallback(*this); _registered = false; }
  }
private:
  void onTopicUpdate() override { _work.ScheduleNow(); }
  Topic<Message> &_topic;
  robot_wq::WorkItem &_work;
  bool _registered{false};
};
}
