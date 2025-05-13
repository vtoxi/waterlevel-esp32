#include "SSD1306DisplayManager.h"

SSD1306DisplayManager::SSD1306DisplayManager(uint8_t width, uint8_t height, int8_t sdaPin, int8_t sclPin)
    : _display(width, height, &Wire, -1), _width(width), _height(height), _sdaPin(sdaPin), _sclPin(sclPin) {}

void SSD1306DisplayManager::begin() {
    Wire.begin(_sdaPin, _sclPin);
    _display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    _display.clearDisplay();
    _display.display();
}

void SSD1306DisplayManager::displayText(const char* text, bool scroll) {
    _display.clearDisplay();
    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, (_height/2)-8);
    _display.cp437(true);
    _display.print(text);
    _display.display();
}

void SSD1306DisplayManager::displayNumber(float value) {
    _display.clearDisplay();
    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, (_height/2)-8);
    _display.cp437(true);
    _display.print(value, 1);
    _display.display();
}

void SSD1306DisplayManager::clear() {
    _display.clearDisplay();
    _display.display();
} 