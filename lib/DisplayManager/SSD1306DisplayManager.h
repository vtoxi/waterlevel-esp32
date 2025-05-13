#pragma once
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "IDisplayManager.h"

class SSD1306DisplayManager : public IDisplayManager {
public:
    SSD1306DisplayManager(uint8_t width, uint8_t height, int8_t sdaPin, int8_t sclPin);
    void begin() override;
    void displayText(const char* text, bool scroll) override;
    void displayNumber(float value) override;
    void clear() override;
private:
    Adafruit_SSD1306 _display;
    uint8_t _width, _height;
    int8_t _sdaPin, _sclPin;
}; 