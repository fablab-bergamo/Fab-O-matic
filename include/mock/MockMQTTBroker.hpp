#if (WOKWI_SIMULATION)
#ifndef MOCKMQTTBROKER_H_
#define MOCKMQTTBROKER_H_

#include <atomic>
#include <functional>
#include <string>

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
    std::string defaultReplies(const std::string &query) const;
    /// @brief set the reply generation function. May be called from a different thread
    /// @param callback
    void configureReplies(std::function<std::string(std::string)> callback);

  private:
    constexpr static uint16_t MQTTPORT = 1883;
    bool is_running = false;
    std::string topic = "";
    std::string payload = "";

    // Maybe set outside the MQTT broker thread
    std::function<std::string(const std::string &)> callback = [this](const std::string &query)
    { return defaultReplies(query); };
    std::atomic<bool> isLocked;

    bool lock() { return !isLocked.exchange(true); }
    void unlock() { isLocked = false; }
  };
} // namespace fablabbg
#endif // MOCKMQTTBROKER_H_
#endif // WOKWI_SIMULATION