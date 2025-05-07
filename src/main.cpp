#include <Arduino.h>
#include "WaterLevelSensor.h"
#include "ConfigManager.h"
#include "WiFiManager.h"
#include "MQTTClient.h"
#include "CustomWebServer.h"
#include "DisplayManager.h"
#include "Logger.h"

// Pin definitions (adjust as needed)
constexpr int TRIGGER_PIN = 5;
constexpr int ECHO_PIN = 18;
constexpr float TANK_HEIGHT_CM = 100.0f; // Set your tank height in cm

WaterLevelSensor sensor(TRIGGER_PIN, ECHO_PIN, TANK_HEIGHT_CM);
ConfigManager configManager;
WiFiManager wifiManager;
MQTTClient mqttClient;
CustomWebServer webServer;

Config config;

// Define your pins
#define DATA_PIN 23
#define CLK_PIN 18
#define CS_PIN 5
#define NUM_MATRICES 4

DisplayManager display(DATA_PIN, CLK_PIN, CS_PIN, NUM_MATRICES);

#define LED_PIN 2 // Onboard LED for most ESP32 dev boards

enum LedState { LED_OFF, LED_ON, LED_BLINK_SLOW, LED_BLINK_FAST };
LedState ledState = LED_OFF;
unsigned long lastLedToggle = 0;
const unsigned long BLINK_INTERVAL_SLOW = 500;  // ms
const unsigned long BLINK_INTERVAL_FAST = 150;  // ms
bool ledOn = false;

void setLedState(LedState state) {
    ledState = state;
    if (state == LED_ON) {
        digitalWrite(LED_PIN, HIGH);
    } else if (state == LED_OFF) {
        digitalWrite(LED_PIN, LOW);
    }
    // For blinking, let loop() handle toggling
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
    // LED_ON and LED_OFF are handled immediately in setLedState
}

String getUniqueAPSSID(const String& prefix = "WL-") {
    uint64_t chipid = ESP.getEfuseMac();
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
    Logger::begin();
    delay(100);

    pinMode(LED_PIN, OUTPUT);
    setLedState(LED_BLINK_SLOW); // Start with slow blink (connecting)

    configManager.load(config);

    logInfo("Attempting WiFi connection...");
    if (!wifiManager.connect(config)) {
        setLedState(LED_BLINK_FAST); // AP mode, blink fast
        String apSsid = getUniqueAPSSID();
        logInfo("WiFi connect failed. Starting AP: " + apSsid);
        wifiManager.startAP(apSsid.c_str());
        webServer.begin(configManager, sensor);
        logInfo("AP mode started. Awaiting configuration.");
        while (true) {
            handleLed();
            delay(10);
        }
    }

    setLedState(LED_ON); // Connected!
    logInfo("WiFi connected!");
    logInfo("Local IP: " + WiFi.localIP().toString());

    // Start web server (for status/info)
    webServer.begin(configManager, sensor);

    // Optionally, redirect to /connected page on first boot after config
    // (You can add logic to detect first boot after config if desired)

    // Connect to MQTT
    mqttClient.connect(config);

    display.begin();
    display.displayText("Hello!", true); // Scroll "Hello!"
}

unsigned long lastPublish = 0;
const unsigned long publishInterval = 10000; // 10 seconds

void loop() {
    handleLed();
    // MQTT loop
    mqttClient.loop();

    // Periodically publish water level
    unsigned long now = millis();
    if (now - lastPublish > publishInterval) {
        lastPublish = now;
        float level = sensor.getWaterLevelPercent();
        String payload = String(level, 1);
        if (mqttClient.isConnected()) {
            mqttClient.publish(config.mqttTopic, payload);
            Serial.println("Published water level: " + payload + "%");
        } else {
            mqttClient.connect(config); // Try to reconnect if needed
        }
    }

    // No need to call webServer.handleClient() (AsyncWebServer is event-driven)

    // Update display as needed
    display.displayLevel(sensor.getWaterLevelPercent());
    display.update(); // Must be called frequently for animation
}
