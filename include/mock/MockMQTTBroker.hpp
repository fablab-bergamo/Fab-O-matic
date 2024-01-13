#ifndef MOCKMQTTBROKER_H_
#define MOCKMQTTBROKER_H_

#include <atomic>
#include <functional>
#include <string>
#include <queue>

#include "sMQTTBroker.h"
#include "BaseRfidWrapper.hpp"

namespace fablabbg
{
  class MockMQTTBroker : public sMQTTBroker
  {
  public:
    MockMQTTBroker();
    ~MockMQTTBroker() = default;

    bool isRunning() const;
    void start();
    bool onEvent(sMQTTEvent *event);
    const std::string defaultReplies(const std::string &query) const;
    /// @brief set the reply generation function. May be called from a different thread
    /// @param callback
    void configureReplies(std::function<const std::string(const std::string &, const std::string &)> callback);
    size_t processQueries();

    void mainLoop();

  private:
    std::atomic<bool> is_running{false};
    std::string topic = "";
    std::string payload = "";
    struct query
    {
      std::string source_topic{""};
      std::string source_query{""};
      std::string reply_topic{""};
    };
    std::queue<query> queries{};

    std::function<const std::string(const std::string &, const std::string &)> callback = [this](const std::string &topic, const std::string &query)
    { return defaultReplies(query); };

    // Maybe set outside the MQTT broker thread
    std::atomic<bool> isLocked;

    bool lock() { return !isLocked.exchange(true); }
    void unlock() { isLocked = false; }
  };
} // namespace fablabbg
#endif // MOCKMQTTBROKER_H_