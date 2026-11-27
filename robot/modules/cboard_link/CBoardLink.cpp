#ifdef main
#undef main
#endif

#include "robot/common/ModuleBase.hpp"
#include "robot/orb/Topics.hpp"
#include "robot/os/Clock.hpp"
#include "protocol/cboard_link/StateProtocol.hpp"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

namespace
{
using namespace cboard_link;

constexpr char DevicePath[] = "/dev/ttyS1";
constexpr uint64_t StatusTimeoutUs = 100000;
constexpr int PollIntervalMs = 10;
constexpr unsigned MaxReadyPasses = 32;
constexpr int ReopenIntervalMs = 200;

class CBoardLink final : public ModuleBase<CBoardLink>
{
public:
  static int task_spawn(int argc, char *argv[])
  {
    const int taskId = os::Task::spawn("cboard_link", 95, 3072,
                                      &run_trampoline, argc, argv);
    if (taskId < 0) { return -1; }
    __atomic_store_n(&_taskId, taskId, __ATOMIC_RELEASE);
    return wait_until_running();
  }

  static CBoardLink *instantiate(int, char *[]) { return new CBoardLink(); }

  static int print_usage(const char *reason = nullptr)
  {
    if (reason) { fprintf(stderr, "%s\n", reason); }
    printf("usage: cboard_link {start|stop|status}\n");
    return reason ? -1 : 0;
  }

  int print_status() override
  {
    printf("device: %s, replies: %lu, events: %lu, I/O errors: %lu\n", DevicePath,
           static_cast<unsigned long>(__atomic_load_n(&_replies, __ATOMIC_RELAXED)),
           static_cast<unsigned long>(__atomic_load_n(&_events, __ATOMIC_RELAXED)),
           static_cast<unsigned long>(__atomic_load_n(&_errors, __ATOMIC_RELAXED)));
    return 0;
  }

  void run() override
  {
    while (!should_exit())
      {
        const int port = open(DevicePath, O_RDWR | O_NONBLOCK | O_NOCTTY);
        if (port >= 0)
          {
            runPort(port);
            close(port);
          }
        if (!should_exit())
          {
            __atomic_add_fetch(&_errors, 1U, __ATOMIC_RELAXED);
            os::Clock::sleepMilliseconds(ReopenIntervalMs);
          }
      }
  }

private:
  void runPort(int port)
  {
    Decoder decoder;
    uint32_t session = 0;
    uint32_t replyQuery = 0;
    uint64_t revision = 1;
    uint64_t readySince = 0;
    bool ready = false;
    bool eventPending = false;
    bool sendingReply = false;
    uint8_t outgoing[FRAME_BYTES]{};
    size_t sent = FRAME_BYTES;
    unsigned readyPasses = 0;

    while (!should_exit())
      {
        ThicknessControlStatus status{};
        const auto now = os::Clock::nowMicroseconds();
        const bool currentReady = _subscription.copy(status) && status.measurementReady &&
                                  status.timestamp != 0 && status.timestamp <= now &&
                                  now - status.timestamp < StatusTimeoutUs;
        const auto currentReadySince = currentReady ? status.readySince : 0;
        if (currentReady != ready || currentReadySince != readySince)
          {
            ready = currentReady;
            readySince = currentReadySince;
            ++revision;
            eventPending = session != 0;
          }

        if (sent == FRAME_BYTES && (replyQuery != 0 || eventPending))
          {
            sendingReply = replyQuery != 0;
            encode({sendingReply ? Kind::Reply : Kind::Event, session,
                    replyQuery, revision, ready}, outgoing);
            sent = 0;
            replyQuery = 0;
            eventPending = false;
          }

        struct pollfd descriptor{};
        descriptor.fd = port;
        descriptor.events = POLLIN | (sent < FRAME_BYTES ? POLLOUT : 0);
        const int result = poll(&descriptor, 1, PollIntervalMs);
        if (result == 0)
          {
            readyPasses = 0;
            continue;
          }
        if (result < 0 && errno == EINTR)
          {
            os::Clock::sleepMilliseconds(PollIntervalMs);
            continue;
          }
        if (result < 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))) { return; }

        if (descriptor.revents & POLLIN)
          {
            uint8_t incoming[128];
            const ssize_t count = read(port, incoming, sizeof(incoming));
            if (count < 0 && errno != EAGAIN && errno != EINTR) { return; }
            for (ssize_t index = 0; index < count; ++index)
              {
                Frame frame{};
                if (!decoder.feed(incoming[index], frame) || frame.kind != Kind::Query ||
                    frame.session == 0 || frame.query == 0) { continue; }
                if (session != frame.session)
                  {
                    session = frame.session;
                    eventPending = false;
                    // Keep the tail of any partially transmitted frame on the wire.
                    if (sent == 0) { sent = FRAME_BYTES; }
                  }
                replyQuery = frame.query;
              }
          }
        if ((descriptor.revents & POLLOUT) && sent < FRAME_BYTES)
          {
            const ssize_t count = write(port, outgoing + sent, FRAME_BYTES - sent);
            if (count < 0 && errno != EAGAIN && errno != EINTR) { return; }
            if (count > 0)
              {
                sent += static_cast<size_t>(count);
                if (sent == FRAME_BYTES)
                  {
                    auto *counter = sendingReply ? &_replies : &_events;
                    __atomic_add_fetch(counter, 1U, __ATOMIC_RELAXED);
                  }
              }
          }
        // 报文可能逐字节到达。连续就绪达到预算才让出 CPU，覆盖噪声和反复 EAGAIN。
        if (++readyPasses >= MaxReadyPasses)
          {
            os::Clock::sleepMilliseconds(PollIntervalMs);
            readyPasses = 0;
          }
      }
  }

  uorb::Subscription<ThicknessControlStatus> _subscription{thicknessControlStatusTopic()};
  uint32_t _replies{0};
  uint32_t _events{0};
  uint32_t _errors{0};
};
}

extern "C" int cboard_link_main(int argc, char *argv[])
{
  return CBoardLink::main(argc, argv);
}
