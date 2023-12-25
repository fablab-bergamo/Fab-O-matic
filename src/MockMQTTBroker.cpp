#if (WOKWI_SIMULATION)
#include "MockMQTTBroker.hpp"
#include "conf.hpp"

namespace fablabbg
{

  MockMQTTBroker::MockMQTTBroker()
  {
  }

  void MockMQTTBroker::start()
  {
    while (WiFi.status() != WL_CONNECTED)
    { // Wait for the Wi-Fi to connect
      if (conf::debug::ENABLE_LOGS)
        Serial.printf("MQTTBROKER: WiFi status changed to %d\r\n", WiFi.status());
      this->is_running = false;
      return;
    }
    if (!this->is_running)
    {
      this->is_running = this->init(MockMQTTBroker::MQTTPORT, true);

      if (conf::debug::ENABLE_LOGS)
        Serial.printf("MQTTBROKER: started with result %d\r\n", this->is_running);
    }
  }
  bool MockMQTTBroker::onEvent(sMQTTEvent *event)
  {
    switch (event->Type())
    {
    case NewClient_sMQTTEventType:
    {
      sMQTTNewClientEvent *e = (sMQTTNewClientEvent *)event;

      if (conf::debug::ENABLE_LOGS)
        Serial.printf("MQTTBROKER: client connected, id:%s\r\n", e->Client()->getClientId().c_str());
    }
    break;
    case Public_sMQTTEventType:
    {
      sMQTTPublicClientEvent *e = (sMQTTPublicClientEvent *)event;
      this->topic = e->Topic();
      this->payload = e->Payload();

      if (conf::debug::ENABLE_LOGS)
        Serial.printf("MQTTBROKER : Received  %s -> %s\r\n", this->topic.c_str(), this->payload.c_str());

      std::string reply = this->fakeReply();
      std::string topic_reply = this->topic + "/reply";
      this->publish(topic_reply, reply, 0, false);
    }
    break;
    case RemoveClient_sMQTTEventType:
    {
      sMQTTRemoveClientEvent *e = (sMQTTRemoveClientEvent *)event;

      if (conf::debug::ENABLE_LOGS)
        Serial.printf("MQTTBROKER: removed client id: %s\r\n", e->Client()->getClientId().c_str());
    }
    break;
    case LostConnect_sMQTTEventType:
      this->is_running = false;
      break;
    }
    return true;
  }

  bool MockMQTTBroker::isRunning() const
  {
    return this->is_running;
  }

  /// @brief Returns a fake server reply for testing purposes
  /// @return json payload
  std::string MockMQTTBroker::fakeReply() const
  {
    if (this->payload.find("checkmachine") != std::string::npos)
    {
      return "{\"request_ok\":true,\"is_valid\":true,\"allowed\":true,\"maintenance\":false,\"timeout_min\":3}";
    }

    if (this->payload.find("maintenance") != std::string::npos)
    {
      return "{\"request_ok\":true}";
    }

    if (this->payload.find("startuse") != std::string::npos)
    {
      return "{\"request_ok\":true}";
    }

    if (this->payload.find("stopuse") != std::string::npos)
    {
      return "{\"request_ok\":true}";
    }

    if (this->payload.find("checkuser") != std::string::npos)
    {
      return "{\"request_ok\":true,\"level\":2,\"name\":\"FAKE USER\",\"is_valid\":true}";
    }

    return "{\"request_ok\":true}";
  }
} // namespace fablabbg
#endif