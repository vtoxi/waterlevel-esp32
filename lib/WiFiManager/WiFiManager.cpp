#include "WiFiManager.h"
#include <WiFi.h>

WiFiManager::WiFiManager() {}

bool WiFiManager::connect(const Config& config) const {
    if (config.wifiSsid.isEmpty() || config.wifiPassword.isEmpty())
        return false;

    WiFi.mode(WIFI_STA);
    // Handle static IP or DHCP
    if (!config.staticIp.isEmpty() && !config.gateway.isEmpty() && !config.subnet.isEmpty()) {
        IPAddress ip, gw, sn;
        ip.fromString(config.staticIp);
        gw.fromString(config.gateway);
        sn.fromString(config.subnet);
        WiFi.config(ip, gw, sn);
        if (!config.hostname.isEmpty()) {
            WiFi.setHostname(config.hostname.c_str());
        }
    } else {
        // Use DHCP (do not call WiFi.config)
        if (!config.hostname.isEmpty()) {
            WiFi.setHostname(config.hostname.c_str());
        }
    }
    WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());

    unsigned long startAttemptTime = millis();
    const unsigned long timeout = 15000; // 15 seconds

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
        delay(200);
    }

    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::startAP(const char* apSsid) const {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid);
}

bool WiFiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}
