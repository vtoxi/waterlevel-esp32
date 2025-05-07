#pragma once
#include <WString.h>

struct Config {
    String wifiSsid;
    String wifiPassword;
    String mqttServer;
    int mqttPort;
    String mqttUser;
    String mqttPassword;
    String mqttTopic;
    float tankDepth = 100.0f;
    String outputUnit = "cm";
    float sensorOffset = 0.0f;
    float sensorFull = 0.0f;
    int displayBrightness = 8;
    String displayMode = "level";
    String staticIp = "";
    String gateway = "";
    String subnet = "";
    String hostname = "";
    int alertLow = 0;
    int alertHigh = 100;
    String alertMethod = "mqtt";
    String deviceName = "";
    String otaEnabled = "off";
};

class ConfigManager {
public:
    ConfigManager();

    bool load(Config& config) const;
    bool save(const Config& config) const;
    void reset() const;
};
