#if (WOKWI_SIMULATION)
#ifndef MOCKMQTTBROKER_H_
#define MOCKMQTTBROKER_H_

#include "sMQTTBroker.h"
#include "BaseRfidWrapper.hpp"

namespace fablabbg
{
  class MockMQTTBroker : public sMQTTBroker
  {
  private:
    constexpr static uint16_t MQTTPORT = 1883;
    bool is_running = false;
    std::string topic = "";
    std::string payload = "";

  public:
    MockMQTTBroker();
    ~MockMQTTBroker() = default;

    bool isRunning() const;
    void start();
    bool onEvent(sMQTTEvent *event);
    std::string fakeReply() const;
  };
} // namespace fablabbg
#endif // MOCKMQTTBROKER_H_
#endif // WOKWI_SIMULATION