#include "ConfigManager.h"
#include <Preferences.h>

#define PREF_NAMESPACE "waterlevel"

ConfigManager::ConfigManager() {}

bool ConfigManager::load(Config& config) const {
    Preferences prefs;
    if (!prefs.begin(PREF_NAMESPACE, true)) return false;

    config.wifiSsid = prefs.getString("wifiSsid", "");
    config.wifiPassword = prefs.getString("wifiPassword", "");
    config.mqttServer = prefs.getString("mqttServer", "");
    config.mqttPort = prefs.getInt("mqttPort", 1883);
    config.mqttUser = prefs.getString("mqttUser", "");
    config.mqttPassword = prefs.getString("mqttPassword", "");
    config.mqttTopic = prefs.getString("mqttTopic", "home/waterlevel");
    config.tankDepth = prefs.getFloat("tankDepth", 100.0f);
    config.outputUnit = prefs.getString("outputUnit", "cm");

    prefs.end();
    return true;
}

bool ConfigManager::save(const Config& config) const {
    Preferences prefs;
    if (!prefs.begin(PREF_NAMESPACE, false)) return false;

    prefs.putString("wifiSsid", config.wifiSsid);
    prefs.putString("wifiPassword", config.wifiPassword);
    prefs.putString("mqttServer", config.mqttServer);
    prefs.putInt   ("mqttPort", config.mqttPort);
    prefs.putString("mqttUser", config.mqttUser);
    prefs.putString("mqttPassword", config.mqttPassword);
    prefs.putString("mqttTopic", config.mqttTopic);
    prefs.putFloat("tankDepth", config.tankDepth);
    prefs.putString("outputUnit", config.outputUnit);

    prefs.end();
    return true;
}

void ConfigManager::reset() const {
    Preferences prefs;
    if (prefs.begin(PREF_NAMESPACE, false)) {
        prefs.clear();
        prefs.end();
    }
}
