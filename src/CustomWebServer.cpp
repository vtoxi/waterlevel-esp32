#include "CustomWebServer.h"
#include "ConfigManager.h"
#include <ArduinoJson.h>

CustomWebServer::CustomWebServer(Config& config) : server(80), config(config) {}

void CustomWebServer::begin() {
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Water Level Sensor API");
    });

    server.on("/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        StaticJsonDocument<512> doc;
        doc["mqttServer"] = config.mqttServer;
        doc["mqttPort"] = config.mqttPort;
        doc["mqttUser"] = config.mqttUser;
        doc["mqttPassword"] = config.mqttPassword;
        doc["mqttTopic"] = config.mqttTopic;
        doc["deviceName"] = config.deviceName;
        doc["tankDepth"] = config.tankDepth;
        doc["outputUnit"] = config.outputUnit;
        doc["sensorOffset"] = config.sensorOffset;
        doc["sensorFull"] = config.sensorFull;
        doc["displayBrightness"] = config.displayBrightness;
        doc["displayMode"] = config.displayMode;
        doc["alertLow"] = config.alertLow;
        doc["alertHigh"] = config.alertHigh;
        doc["alertMethod"] = config.alertMethod;
        doc["otaEnabled"] = config.otaEnabled;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
        request->send(200);
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, (char*)data);

        if (!error) {
            strlcpy(config.mqttServer, doc["mqttServer"] | "", sizeof(config.mqttServer));
            config.mqttPort = doc["mqttPort"] | 1883;
            strlcpy(config.mqttUser, doc["mqttUser"] | "", sizeof(config.mqttUser));
            strlcpy(config.mqttPassword, doc["mqttPassword"] | "", sizeof(config.mqttPassword));
            strlcpy(config.mqttTopic, doc["mqttTopic"] | "", sizeof(config.mqttTopic));
            strlcpy(config.deviceName, doc["deviceName"] | "", sizeof(config.deviceName));
            config.tankDepth = doc["tankDepth"] | 100.0f;
            strlcpy(config.outputUnit, doc["outputUnit"] | "cm", sizeof(config.outputUnit));
            config.sensorOffset = doc["sensorOffset"] | 0.0f;
            config.sensorFull = doc["sensorFull"] | 100.0f;
            config.displayBrightness = doc["displayBrightness"] | 8;
            strlcpy(config.displayMode, doc["displayMode"] | "level", sizeof(config.displayMode));
            config.alertLow = doc["alertLow"] | 20;
            config.alertHigh = doc["alertHigh"] | 80;
            strlcpy(config.alertMethod, doc["alertMethod"] | "mqtt", sizeof(config.alertMethod));
            strlcpy(config.otaEnabled, doc["otaEnabled"] | "off", sizeof(config.otaEnabled));

            ConfigManager::saveConfig(config);
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        }
    });

    server.begin();
} 