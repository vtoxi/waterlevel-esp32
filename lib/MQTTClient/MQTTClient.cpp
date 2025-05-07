#include "MQTTClient.h"
#include <PubSubClient.h>
#include <WiFi.h>

namespace {
    WiFiClient wifiClient;
    PubSubClient mqttClient(wifiClient);
}

MQTTClient::MQTTClient()
    : _port(1883)
{}

bool MQTTClient::connect(const Config& config) {
    _server = config.mqttServer;
    _port = config.mqttPort;
    _user = config.mqttUser;
    _password = config.mqttPassword;
    _clientId = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    mqttClient.setServer(_server.c_str(), _port);

    if (mqttClient.connected())
        return true;

    // Try to connect (short-circuit if no server)
    return (!_server.isEmpty() && mqttClient.connect(
        _clientId.c_str(),
        _user.isEmpty() ? nullptr : _user.c_str(),
        _password.isEmpty() ? nullptr : _password.c_str()
    ));
}

bool MQTTClient::publish(const String& topic, const String& payload) {
    return mqttClient.connected() && mqttClient.publish(topic.c_str(), payload.c_str());
}

void MQTTClient::loop() {
    mqttClient.loop();
}

bool MQTTClient::isConnected() const {
    return mqttClient.connected();
}
