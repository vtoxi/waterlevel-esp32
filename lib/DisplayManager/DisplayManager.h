#pragma once
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include "IDisplayManager.h"

class DisplayManager : public IDisplayManager {
public:
    DisplayManager(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t numDevices);
    void begin();
    void displayText(const char* text, bool scroll = true);
    void displayNumber(float value) override;
    void displayLevel(float percent);
    void update();
    void setHardwareType(uint8_t hwType);
    void setBrightness(int value);
    void clear() override;
private:
    MD_Parola _parola;
    uint8_t _numDevices;
    String _currentText;
    uint8_t _hardwareType = MD_MAX72XX::FC16_HW;
    uint8_t _dataPin, _clkPin, _csPin;
};
