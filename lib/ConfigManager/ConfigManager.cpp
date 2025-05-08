#include "ConfigManager.h"
#include <EEPROM.h>

const char* ConfigManager::PREF_NAMESPACE = "waterlevel";
const int ConfigManager::EEPROM_SIZE = 512;
const int ConfigManager::CONFIG_VERSION = 1;

ConfigManager::ConfigManager() {
    EEPROM.begin(EEPROM_SIZE);
}

bool ConfigManager::load(Config& config) const {
    if (EEPROM.read(0) != CONFIG_VERSION) {
        return false;
    }

    int addr = 1;
    EEPROM.get(addr, config);
    return true;
}

bool ConfigManager::save(const Config& config) const {
    EEPROM.write(0, CONFIG_VERSION);
    int addr = 1;
    EEPROM.put(addr, config);
    return EEPROM.commit();
}

void ConfigManager::reset() const {
    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}

bool ConfigManager::saveConfig(const Config& config) {
    EEPROM.begin(sizeof(Config));
    EEPROM.put(0, config);
    return EEPROM.commit();
}

bool ConfigManager::loadConfig(Config& config) {
    EEPROM.begin(sizeof(Config));
    EEPROM.get(0, config);
    return config.magic == CONFIG_MAGIC;
}

void ConfigManager::resetConfig(Config& config) {
    config.magic = CONFIG_MAGIC;
    strlcpy(config.wifiSSID, "", sizeof(config.wifiSSID));
    strlcpy(config.wifiPassword, "", sizeof(config.wifiPassword));
    strlcpy(config.mqttServer, "", sizeof(config.mqttServer));
    config.mqttPort = 1883;
    strlcpy(config.mqttUser, "", sizeof(config.mqttUser));
    strlcpy(config.mqttPassword, "", sizeof(config.mqttPassword));
    strlcpy(config.mqttTopic, "home/waterlevel", sizeof(config.mqttTopic));
    config.tankDepth = 100.0f;
    strlcpy(config.outputUnit, "cm", sizeof(config.outputUnit));
    config.sensorOffset = 0.0f;
    config.sensorFull = 100.0f;
    config.displayBrightness = 8;
    strlcpy(config.displayMode, "level", sizeof(config.displayMode));
    strlcpy(config.staticIp, "", sizeof(config.staticIp));
    strlcpy(config.gateway, "", sizeof(config.gateway));
    strlcpy(config.subnet, "", sizeof(config.subnet));
    strlcpy(config.hostname, "", sizeof(config.hostname));
    config.alertLow = 20;
    config.alertHigh = 80;
    strlcpy(config.alertMethod, "mqtt", sizeof(config.alertMethod));
    strlcpy(config.deviceName, "", sizeof(config.deviceName));
    strlcpy(config.otaEnabled, "off", sizeof(config.otaEnabled));
}
