#pragma once
#include <Arduino.h>

class LogManager {
public:
    static void initLogFile();
    static void logLevelReading(unsigned long timestamp, float distance, float percent, float levelCm, float levelIn, float liters, float gallons);
}; 