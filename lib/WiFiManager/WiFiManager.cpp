#include "WiFiManager.h"
#include <ESP8266WiFi.h>

WiFiManager::WiFiManager() : apMode(false), lastReconnectAttempt(0) {}

void WiFiManager::begin(const Config& config) {
    if (strlen(config.wifiSSID) > 0) {
        if (!connect(config)) {
            startAP("WaterLevel-AP");
        }
    } else {
        startAP("WaterLevel-AP");
    }
}

void WiFiManager::loop() {
    if (!apMode && !isConnected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            Serial.println("WiFi disconnected. Attempting to reconnect...");
            WiFi.reconnect();
        }
    }
}

bool WiFiManager::connect(const Config& config) {
    if (strlen(config.wifiSSID) == 0) {
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifiSSID, config.wifiPassword);

    // Try to connect for 30 seconds
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
        Serial.println("IP address: " + WiFi.localIP().toString());
        return true;
    }

    Serial.println("WiFi connection failed");
    return false;
}

void WiFiManager::startAP(const char* ssid) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
    apMode = true;
    Serial.println("AP Mode started");
    Serial.println("AP IP address: " + WiFi.softAPIP().toString());
}

void WiFiManager::stopAP() {
    WiFi.softAPdisconnect(true);
}

bool WiFiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getLocalIP() const {
    return WiFi.localIP().toString();
}
