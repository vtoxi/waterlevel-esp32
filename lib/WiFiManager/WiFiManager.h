#pragma once
#include <ESP8266WiFi.h>
#include "Config.h"

class WiFiManager {
public:
    WiFiManager();
    void begin(const Config& config);
    void loop();
    bool connect(const Config& config);
    void startAP(const char* ssid);
    void stopAP();
    bool isConnected() const;
    String getLocalIP() const;
private:
    bool apMode;
    unsigned long lastReconnectAttempt;
};
