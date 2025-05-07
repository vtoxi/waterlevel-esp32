#pragma once
#include <MD_Parola.h>
#include <MD_MAX72XX.h>

class DisplayManager {
public:
    DisplayManager(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t numDevices);
    void begin();
    void displayText(const char* text, bool scroll = true);
    void displayNumber(long number);
    void displayLevel(float percent);
    void update();
private:
    MD_Parola _parola;
    uint8_t _numDevices;
    String _currentText;
};
