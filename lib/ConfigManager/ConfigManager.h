#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "Config.h"

#define CONFIG_MAGIC 0x574C  // "WL" in hex
#define MAX_STRING_LENGTH 32

class ConfigManager {
public:
    ConfigManager();
    bool load(Config&) const;
    bool save(const Config&) const;
    void reset() const;
    static bool saveConfig(const Config& config);
    static bool loadConfig(Config& config);
    static void resetConfig(Config& config);

private:
    static const char* PREF_NAMESPACE;
    static const int EEPROM_SIZE;
    static const int CONFIG_VERSION;
};
