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
    config.sensorOffset = prefs.getFloat("sensorOffset", 0.0f);
    config.sensorFull = prefs.getFloat("sensorFull", 0.0f);
    config.displayBrightness = prefs.getInt("displayBrightness", 8);
    config.displayMode = prefs.getString("displayMode", "level");
    config.displayHardwareType = prefs.getString("displayHardwareType", "FC16_HW");
    config.displayScrollEnabled = prefs.getBool("displayScrollEnabled", true);
    config.staticIp = prefs.getString("staticIp", "");
    config.gateway = prefs.getString("gateway", "");
    config.subnet = prefs.getString("subnet", "");
    config.hostname = prefs.getString("hostname", "");
    config.alertLow = prefs.getInt("alertLow", 0);
    config.alertHigh = prefs.getInt("alertHigh", 100);
    config.alertMethod = prefs.getString("alertMethod", "mqtt");
    config.deviceName = prefs.getString("deviceName", "");
    config.otaEnabled = prefs.getString("otaEnabled", "off");
    config.sensorReadInterval = prefs.getInt("sensorReadInterval", 1);
    config.tankDepthUnit = prefs.getString("tankDepthUnit", "cm");
    config.tankShape = prefs.getString("tankShape", "rectangle");
    config.tankWidth = prefs.getFloat("tankWidth", 0.0f);
    config.tankLength = prefs.getFloat("tankLength", 0.0f);
    config.tankDiameter = prefs.getFloat("tankDiameter", 0.0f);
    config.volumeUnit = prefs.getString("volumeUnit", "L");

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
    prefs.putFloat ("tankDepth", config.tankDepth);
    prefs.putString("outputUnit", config.outputUnit);
    prefs.putFloat ("sensorOffset", config.sensorOffset);
    prefs.putFloat ("sensorFull", config.sensorFull);
    prefs.putInt   ("displayBrightness", config.displayBrightness);
    prefs.putString("displayMode", config.displayMode);
    prefs.putString("displayHardwareType", config.displayHardwareType);
    prefs.putBool("displayScrollEnabled", config.displayScrollEnabled);
    prefs.putString("staticIp", config.staticIp);
    prefs.putString("gateway", config.gateway);
    prefs.putString("subnet", config.subnet);
    prefs.putString("hostname", config.hostname);
    prefs.putInt   ("alertLow", config.alertLow);
    prefs.putInt   ("alertHigh", config.alertHigh);
    prefs.putString("alertMethod", config.alertMethod);
    prefs.putString("deviceName", config.deviceName);
    prefs.putString("otaEnabled", config.otaEnabled);
    prefs.putInt   ("sensorReadInterval", config.sensorReadInterval);
    prefs.putString("tankDepthUnit", config.tankDepthUnit);
    prefs.putString("tankShape", config.tankShape);
    prefs.putFloat("tankWidth", config.tankWidth);
    prefs.putFloat("tankLength", config.tankLength);
    prefs.putFloat("tankDiameter", config.tankDiameter);
    prefs.putString("volumeUnit", config.volumeUnit);

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
