#pragma once
class IDisplayManager {
public:
    virtual void begin() = 0;
    virtual void displayText(const char* text, bool scroll) = 0;
    virtual void displayNumber(float value) = 0;
    virtual void clear() = 0;
    virtual ~IDisplayManager() {}
}; 