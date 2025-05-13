#include "LogManager.h"
#include <LittleFS.h>

void LogManager::initLogFile() {
    if (!LittleFS.exists("/level_log.csv")) {
        File f = LittleFS.open("/level_log.csv", "w");
        if (f) {
            f.println("timestamp,distance_cm,percent,level_cm,level_in,liters,gallons");
            f.close();
        }
    }
}

void LogManager::logLevelReading(unsigned long timestamp, float distance, float percent, float levelCm, float levelIn, float liters, float gallons) {
    File f = LittleFS.open("/level_log.csv", "a");
    if (f) {
        f.printf("%lu,%.2f,%.1f,%.2f,%.2f,%.2f,%.2f\n", timestamp, distance, percent, levelCm, levelIn, liters, gallons);
        f.close();
    }
} 