#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <list>
#include <deque>
#include <memory>
#include "../lib/WaterLevelSensor/WaterLevelSensor.h"
#include "../lib/ConfigManager/ConfigManager.h"
#include "../lib/WiFiManager/WiFiManager.h"
#include "../lib/MQTTClient/MQTTClient.h"
#include "../lib/CustomWebServer/CustomWebServer.h"
#include "../lib/Logger/Logger.h"
#include "../lib/Config/Config.h"
#include <MD_MAX72XX.h>
#include <SPI.h>

// Pin definitions for Wemos D1 Mini Pro
constexpr int TRIGGER_PIN = D4;  // GPIO5
constexpr int ECHO_PIN = D8;     // GPIO4
constexpr float TANK_HEIGHT_CM = 100.0f; // Set your tank height in cm

// Define pins for MAX7219 7-segment display
#define CLK_PIN  D5  // GPIO14
#define CS_PIN   D6  // GPIO15
#define DATA_PIN D7  // GPIO13

#define LED_PIN D4  // Built-in LED on Wemos D1 Mini Pro

// Global variables
WaterLevelSensor sensor;
ConfigManager configManager;
WiFiManager wifiManager;
MQTTClient* mqttClient = nullptr;
CustomWebServer webServer;
Config config;

// Create MAX7219 object
MD_MAX72XX display = MD_MAX72XX(MD_MAX72XX::FC16_HW, CS_PIN, 1);

enum LedState { LED_OFF, LED_ON, LED_BLINK_SLOW, LED_BLINK_FAST };
LedState ledState = LED_OFF;
unsigned long lastLedToggle = 0;
const unsigned long BLINK_INTERVAL_SLOW = 500;  // ms
const unsigned long BLINK_INTERVAL_FAST = 150;  // ms
bool ledOn = false;

// Function declarations
void setLedState(LedState state);
void handleLed();
String getUniqueAPSSID(const String& prefix = "WL-");
void logInfo(const String& msg);

// Function implementations
void setLedState(LedState state) {
    ledState = state;
    if (state == LED_ON) {
        digitalWrite(LED_PIN, HIGH);
    } else if (state == LED_OFF) {
        digitalWrite(LED_PIN, LOW);
    }
}

void handleLed() {
    unsigned long now = millis();
    if (ledState == LED_BLINK_SLOW) {
        if (now - lastLedToggle > BLINK_INTERVAL_SLOW) {
            ledOn = !ledOn;
            digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
            lastLedToggle = now;
        }
    } else if (ledState == LED_BLINK_FAST) {
        if (now - lastLedToggle > BLINK_INTERVAL_FAST) {
            ledOn = !ledOn;
            digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
            lastLedToggle = now;
        }
    }
}

String getUniqueAPSSID(const String& prefix) {
    uint32_t chipid = ESP.getChipId();
    // Use last 2 bytes (4 hex digits) for brevity and uniqueness
    char uniquePart[5];
    snprintf(uniquePart, sizeof(uniquePart), "%04X", (uint16_t)(chipid & 0xFFFF));
    return prefix + uniquePart;
}

void logInfo(const String& msg) {
    Serial.println("[INFO] " + msg);
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nStarting Water Level Monitor...");

    // Initialize the file system
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount file system");
        return;
    }

    // Load configuration
    if (!configManager.loadConfig(config)) {
        Serial.println("Failed to load config, using defaults");
        configManager.resetConfig(config);
        configManager.saveConfig(config);
    }

    // Initialize WiFi
    wifiManager.begin(config);

    // Initialize web server
    webServer.begin(configManager, sensor);

    // Initialize MQTT if configured
    if (strlen(config.mqttServer) > 0) {
        mqttClient = new MQTTClient(config);
        mqttClient->begin();
    }

    // Initialize sensor
    sensor.begin();

    // Initialize MAX7219
    display.begin();
    display.control(MD_MAX72XX::INTENSITY, 8); // Set brightness (0-15)
    display.clear();
    
    // Display "HELLO" on startup
    const char* hello = "HELLO";
    for (size_t i = 0; i < strlen(hello); i++) {
        display.setChar(7-i, hello[i]);
    }
    delay(1000);
    display.clear();
}

void loop() {
    handleLed();
    
    // Handle WiFi connection
    wifiManager.loop();

    // Handle MQTT connection and messages
    if (mqttClient) {
        mqttClient->loop();
    }

    // Update sensor readings
    sensor.loop();

    // Publish sensor data to MQTT if connected
    static unsigned long lastPublish = 0;
    if (mqttClient && millis() - lastPublish > 5000) {
        char payload[32];
        snprintf(payload, sizeof(payload), "%.1f", sensor.getWaterLevelPercent());
        mqttClient->publish(config.mqttTopic, payload);
        lastPublish = millis();
    }

    // Update display
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 10000) {
        lastDisplayUpdate = millis();
        float level = sensor.getWaterLevelPercent();
        char displayText[9];
        snprintf(displayText, sizeof(displayText), "%.1f%%", level);
        display.clear();
        
        // Display the water level
        for (size_t i = 0; i < strlen(displayText); i++) {
            display.setChar(7-i, displayText[i]);
        }
    }

    delay(10);
}
