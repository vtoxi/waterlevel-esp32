#pragma once
#include <WString.h>
#include "ConfigManager.h"

class WiFiManager {
public:
    WiFiManager();
    bool connect(const Config& config) const;
    void startAP(const char* apSsid = "WaterLevelSetup") const;
    bool isConnected() const;
};
