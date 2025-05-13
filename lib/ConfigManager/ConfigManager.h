#pragma once
#include <WString.h>

struct Config {
    String wifiSsid = "CodeRunner";
    String wifiPassword = "qWe123!@#";
    String mqttServer;
    int mqttPort;
    String mqttUser;
    String mqttPassword;
    String mqttTopic;
    float tankDepth = 100.0f;
    String tankDepthUnit = "cm";
    String outputUnit = "cm";
    float sensorOffset = 0.0f;
    float sensorFull = 0.0f;
    int displayBrightness = 8;
    String displayMode = "level";
    String displayHardwareType = "FC16_HW";
    bool displayScrollEnabled = true;
    String staticIp = "";
    String gateway = "";
    String subnet = "";
    String hostname = "";
    int alertLow = 0;
    int alertHigh = 100;
    String alertMethod = "mqtt";
    String deviceName = "";
    String otaEnabled = "off";
    int sensorReadInterval = 1; // JSN-SR04T reading interval in seconds (min 1)
    String tankShape = "rectangle"; // or "cylinder"
    float tankDiameter = 0.0f; // for cylinder, in cm
    float tankWidth = 0.0f;    // for rectangle, in cm
    float tankLength = 0.0f;   // for rectangle, in cm
    String volumeUnit = "L"; // for liters or gallons
    String displayType = "matrix"; // 'matrix', 'sevensegment', or 'ssd1306'
    int ssd1306Width = 128;
    int ssd1306Height = 64;
};

class ConfigManager {
public:
    ConfigManager();

    bool load(Config& config) const;
    bool save(const Config& config) const;
    void reset() const;
};
