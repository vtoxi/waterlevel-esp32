#include "MQTTClient.h"
#include <ESP8266WiFi.h>

WiFiClient wifiClient;
PubSubClient MQTTClient::mqttClient(wifiClient);

MQTTClient::MQTTClient(Config& config) : config(config) {
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
}

void MQTTClient::begin() {
    if (strlen(config.mqttServer) > 0) {
        mqttClient.setServer(config.mqttServer, config.mqttPort);
        if (mqttClient.connect("WaterLevel", config.mqttUser, config.mqttPassword)) {
            mqttClient.subscribe(config.mqttTopic);
        }
    }
}

void MQTTClient::loop() {
    if (mqttClient.connected()) {
        mqttClient.loop();
    } else if (strlen(config.mqttServer) > 0) {
        if (mqttClient.connect("WaterLevel", config.mqttUser, config.mqttPassword)) {
            mqttClient.subscribe(config.mqttTopic);
        }
    }
}

void MQTTClient::publish(const char* topic, const char* payload) {
    if (mqttClient.connected()) {
        mqttClient.publish(topic, payload);
    }
}

void MQTTClient::callback(char* topic, byte* payload, unsigned int length) {
    // Handle incoming messages
    // For now, we just print them
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    Serial.printf("Message arrived [%s]: %s\n", topic, message);
}
