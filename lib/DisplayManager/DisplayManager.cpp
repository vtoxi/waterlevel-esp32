#include "DisplayManager.h"
#include <Arduino.h>

DisplayManager::DisplayManager(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t numDevices)
    : _parola(MD_MAX72XX::FC16_HW, dataPin, clkPin, csPin, numDevices), _numDevices(numDevices) {}

void DisplayManager::begin() {
    _parola.begin();
    _parola.setIntensity(5); // 0-15
    _parola.displayClear();
}

void DisplayManager::displayText(const char* text, bool scroll) {
    _currentText = text;
    _parola.displayClear();
    _parola.displayText(_currentText.c_str(), PA_CENTER, 50, 2000, scroll ? PA_SCROLL_LEFT : PA_PRINT, PA_NO_EFFECT);
}

void DisplayManager::displayNumber(long number) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", number);
    displayText(buf, false);
}

void DisplayManager::displayLevel(float percent) {
    char buf[16];
    snprintf(buf, sizeof(buf), "Level: %.1f%%", percent);
    displayText(buf, true);
}

void DisplayManager::update() {
    _parola.displayAnimate();
}
