#include "robot/work_queue/WorkQueue.hpp"
#include "robot/work_queue/ScheduledWorkItem.hpp"
#include "robot/orb/SubscriptionCallback.hpp"
#include "robot/os/Clock.hpp"
#include <atomic>
#include <assert.h>
#include <stdio.h>
#include <thread>

struct Message { unsigned value{}; };
class Item final : public robot_wq::ScheduledWorkItem
{
public:
  explicit Item(robot_wq::WorkQueue &queue) : ScheduledWorkItem(queue) {}
  void Run() override
  {
    ++runs;
    entered = true;
    // Intentional blocking only in this test, to exercise stop timeout.
    while (hold.load()) { os::Clock::sleepMilliseconds(1); }
  }
  std::atomic<unsigned> runs{0};
  std::atomic<bool> hold{false};
  std::atomic<bool> entered{false};
};
template<class Predicate>
static void waitFor(Predicate predicate, uint32_t timeoutMs = 1000)
{
  const uint64_t deadline = os::Clock::nowMicroseconds() + timeoutMs * 1000ULL;
  while (!predicate() && os::Clock::nowMicroseconds() < deadline)
    { os::Clock::sleepMilliseconds(1); }
  assert(predicate());
}

int main()
{
  robot_wq::WorkQueue queue("test", 105, 8192);
  Item item(queue);
  assert(item.InitSchedule(30000));
  uorb::Topic<Message> topic;
  uorb::SubscriptionCallbackWorkItem<Message> callback(topic, item);
  assert(callback.registerCallback());
  // Initialization must not execute an incompletely initialized module.
  assert(topic.publish(Message{}));
  os::Clock::sleepMilliseconds(10);
  assert(item.runs == 0);
  item.ScheduleOnInterval(0); // Activate without a periodic timer.
  item.ScheduleDelayed(80000);
  os::Clock::sleepMilliseconds(10);
  assert(item.runs == 0);
  // A new earlier deadline must wake a worker already waiting for a later one.
  assert(topic.publish(Message{}));
  waitFor([&] { return item.runs == 1; });
  for (unsigned i = 0; i < 100; ++i) { assert(topic.publish(Message{i})); }
  os::Clock::sleepMilliseconds(5);
  assert(item.runs == 1); // Rate limit; 100 notifications coalesce to one.
  waitFor([&] { return item.runs == 2; });
  os::Clock::sleepMilliseconds(50);
  assert(item.runs == 2);
  item.ScheduleOnInterval(30000);
  waitFor([&] { return item.runs >= 4; }); // No new topic is necessary.
  item.ScheduleClear();
  assert(item.StopAndWait());
  callback.unregisterCallback();
  unsigned stopped = item.runs;
  assert(topic.publish(Message{}));
  os::Clock::sleepMilliseconds(40);
  assert(item.runs == stopped);

  // A timed-out detach must retain the live object, not destroy the worker.
  Item held(queue);
  assert(held.InitSchedule(0));
  held.ScheduleOnInterval(0);
  held.hold = true;
  held.ScheduleNow();
  waitFor([&] { return held.entered.load(); });
  assert(!held.StopAndWait(10));
  held.hold = false;
  assert(held.StopAndWait());

  // Concurrent publishing/unregistration must not retain stale pointers.
  for (unsigned iteration = 0; iteration < 30; ++iteration)
    {
      Item temporary(queue);
      assert(temporary.InitSchedule(1000));
      temporary.ScheduleOnInterval(0);
      uorb::SubscriptionCallbackWorkItem<Message> subscription(topic, temporary);
      assert(subscription.registerCallback());
      std::thread publisher([&] {
        for (unsigned i = 0; i < 500; ++i) { assert(topic.publish(Message{i})); }
      });
      subscription.unregisterCallback();
      assert(temporary.StopAndWait());
      publisher.join();
    }
  assert(queue.Status().items == 0);
  assert(queue.Shutdown());
  puts("work queue deadlines, coalescing, fallback and safe detach passed");
}
