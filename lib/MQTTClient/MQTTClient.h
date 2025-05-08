#pragma once
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Config.h"

class MQTTClient {
private:
    static PubSubClient mqttClient;
    Config& config;
    void callback(char* topic, byte* payload, unsigned int length);

public:
    MQTTClient(Config& config);
    void begin();
    void loop();
    void publish(const char* topic, const char* payload);
};
