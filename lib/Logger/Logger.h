#pragma once
#include <WString.h>
#include <LittleFS.h>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static void begin();
    static void log(LogLevel level, const String& message);
    static void debug(const String& message);
    static void info(const String& message);
    static void warn(const String& message);
    static void error(const String& message);
    static String getLogs();
    static void clearLogs();

private:
    static const char* LOG_FILE;
    static const size_t MAX_LOG_SIZE = 1024 * 10; // 10KB
    static void rotateLogs();
    static void writeToFile(const String& message);
};
