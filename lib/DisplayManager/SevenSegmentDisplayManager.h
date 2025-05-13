#pragma once
#include <MD_MAX72XX.h>
#include <Arduino.h>
#include "IDisplayManager.h"

class SevenSegmentDisplayManager : public IDisplayManager {
public:
    SevenSegmentDisplayManager(uint8_t dataPin, uint8_t clkPin, uint8_t csPin);
    void begin() override;
    void displayText(const char* text, bool scroll) override;
    void displayNumber(float value) override;
    void clear() override;
private:
    MD_MAX72XX _display;
}; 