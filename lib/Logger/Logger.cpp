#include "Logger.h"
#include <Arduino.h>

namespace {
    const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default:              return "";
        }
    }
}

const char* Logger::LOG_FILE = "/logs.txt";

void Logger::begin() {
    if (!LittleFS.exists(LOG_FILE)) {
        File file = LittleFS.open(LOG_FILE, "w");
        file.close();
    }
}

void Logger::log(LogLevel level, const String& message) {
    // Log to Serial
    Serial.print("[");
    Serial.print(levelToString(level));
    Serial.print("] ");
    Serial.println(message);

    // Log to file
    String logMessage = String("[") + levelToString(level) + "] " + message;
    writeToFile(logMessage);
}

void Logger::writeToFile(const String& message) {
    File file = LittleFS.open(LOG_FILE, "a");
    if (file) {
        file.println(message);
        file.close();
        
        // Check if we need to rotate logs
        if (file.size() > MAX_LOG_SIZE) {
            rotateLogs();
        }
    }
}

void Logger::rotateLogs() {
    // Create backup of current log file
    if (LittleFS.exists(LOG_FILE)) {
        File currentLog = LittleFS.open(LOG_FILE, "r");
        if (currentLog) {
            // Read last 1000 bytes (or less if file is smaller)
            size_t fileSize = currentLog.size();
            size_t readSize = min(fileSize, (size_t)1000);
            currentLog.seek(fileSize - readSize);
            
            String lastLogs = currentLog.readString();
            currentLog.close();
            
            // Clear current log and write last 1000 bytes
            File newLog = LittleFS.open(LOG_FILE, "w");
            if (newLog) {
                newLog.println("=== Log rotation occurred ===");
                newLog.println(lastLogs);
                newLog.close();
            }
        }
    }
}

String Logger::getLogs() {
    String logs;
    File file = LittleFS.open(LOG_FILE, "r");
    if (file) {
        logs = file.readString();
        file.close();
    }
    return logs;
}

void Logger::clearLogs() {
    File file = LittleFS.open(LOG_FILE, "w");
    if (file) {
        file.println("=== Logs cleared ===");
        file.close();
    }
}

void Logger::debug(const String& message) { log(LogLevel::DEBUG, message); }
void Logger::info(const String& message)  { log(LogLevel::INFO, message); }
void Logger::warn(const String& message)  { log(LogLevel::WARN, message); }
void Logger::error(const String& message) { log(LogLevel::ERROR, message); }
