#pragma once

class OTAUpdateManager {
public:
    OTAUpdateManager();
    void begin(const char* hostname = "esp32-waterlevel") const;
    void handle() const;
};
