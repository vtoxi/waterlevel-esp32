#include "SevenSegmentDisplayManager.h"

#define NUM_DIGITS 8
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

SevenSegmentDisplayManager::SevenSegmentDisplayManager(uint8_t dataPin, uint8_t clkPin, uint8_t csPin)
    : _display(HARDWARE_TYPE, dataPin, clkPin, csPin, 1) {}

void SevenSegmentDisplayManager::begin() {
    _display.begin();
    _display.control(MD_MAX72XX::INTENSITY, 5);
    _display.clear();
}

void SevenSegmentDisplayManager::displayNumber(float value) {
    char buf[NUM_DIGITS + 2]; // +1 for possible '.' at end, +1 for null
    dtostrf(value, 0, 1, buf); // 1 decimal place, no padding

    // Find length excluding null terminator
    size_t len = strlen(buf);
    // Work from right to left for right alignment
    int digit = NUM_DIGITS - 1;
    for (int i = len - 1; i >= 0 && digit >= 0;) {
        if (buf[i] == '.') {
            // Set decimal point for previous digit
            if (digit < NUM_DIGITS - 1) {
                // Set the decimal point segment (bit 7) for this digit
                uint8_t col = _display.getColumn(digit + 1);
                _display.setColumn(digit + 1, col | 0x80);
            }
            --i;
            continue;
        }
        // Display the digit/char
        _display.setChar(digit, buf[i]);
        // Clear decimal point by default
        uint8_t col = _display.getColumn(digit);
        _display.setColumn(digit, col & 0x7F);
        --digit;
        --i;
    }
    // Blank remaining digits
    for (; digit >= 0; --digit) {
        _display.setChar(digit, ' ');
        uint8_t col = _display.getColumn(digit);
        _display.setColumn(digit, col & 0x7F);
    }
    _display.update();
}

void SevenSegmentDisplayManager::clear() {
    _display.clear();
    _display.update();
}

void SevenSegmentDisplayManager::displayText(const char* text, bool scroll) {
    // 7-segment cannot display text, so display as float if possible
    displayNumber(atof(text));
} 