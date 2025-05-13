#include "DisplayManager.h"
#include <Arduino.h>

DisplayManager::DisplayManager(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t numDevices)
    : _parola(MD_MAX72XX::FC16_HW, dataPin, clkPin, csPin, numDevices),
      _numDevices(numDevices),
      _hardwareType(MD_MAX72XX::FC16_HW),
      _dataPin(dataPin), _clkPin(clkPin), _csPin(csPin) {}

void DisplayManager::setHardwareType(uint8_t hwType) {
    _hardwareType = hwType;
}

void DisplayManager::begin() {
    new (&_parola) MD_Parola((MD_MAX72XX::moduleType_t)_hardwareType, _dataPin, _clkPin, _csPin, _numDevices);
    _parola.begin();
    _parola.setIntensity(5); // 0-15
    _parola.displayClear();
}

void DisplayManager::displayText(const char* text, bool scroll) {
    _currentText = text;
    _parola.displayClear();
    _parola.displayText(_currentText.c_str(), PA_CENTER, 50, 2000, scroll ? PA_SCROLL_LEFT : PA_PRINT, PA_NO_EFFECT);
}

void DisplayManager::displayNumber(float value) {
    char buf[16];
    dtostrf(value, 0, 1, buf);
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

void DisplayManager::setBrightness(int value) {
    int v = value;
    if (v < 0) v = 0;
    if (v > 15) v = 15;
    _parola.setIntensity(v);
}

void DisplayManager::clear() {
    _parola.displayClear();
}
