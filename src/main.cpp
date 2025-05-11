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
constexpr float TANK_HEIGHT_CM = 300.0f; // Set your tank height in cm

WaterLevelSensor sensor(TRIGGER_PIN, ECHO_PIN, TANK_HEIGHT_CM);
ConfigManager configManager;
WiFiManager wifiManager;
MQTTClient mqttClient;
CustomWebServer webServer;

Config config;

// Define your pins
#define DATA_PIN 23
#define CLK_PIN 19
#define CS_PIN 21
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

void showLogOnDisplay(const String& log) {
    static unsigned long lastDisplay = 0;
    unsigned long now = millis();
    // Only update if at least 500ms since last update to avoid flicker
    if (now - lastDisplay > 500) {
        display.displayText(log.c_str(), true); // Scroll the log message
        lastDisplay = now;
    }
}

void setup() {
    Serial.begin(9600);
    Logger::begin();
    // Logger::setDisplayCallback(showLogOnDisplay); // Disabled: do not print logs on the display
    delay(100);

    Serial.println("[BOOT] Starting Water Level Monitor...");
    pinMode(LED_PIN, OUTPUT);
    setLedState(LED_BLINK_SLOW); // Start with slow blink (connecting)

    Serial.println("[CONFIG] Loading configuration...");
    if (configManager.load(config)) {
        Serial.println("[CONFIG] Configuration loaded successfully.");
    } else {
        Serial.println("[CONFIG] Failed to load configuration!");
    }

    Serial.println("[SENSOR] Initializing sensor...");
    float testDistance = sensor.readDistanceCm();
    if (testDistance >= 0) {
        Serial.println("[SENSOR] Sensor connected. Distance: " + String(testDistance, 1) + " cm");
    } else {
        Serial.println("[SENSOR] Sensor NOT connected! (timeout or error)");
    }

    logInfo("Attempting WiFi connection...");
    bool wifiOk = wifiManager.connect(config);
    if (!wifiOk) {
        setLedState(LED_BLINK_FAST); // AP mode, blink fast
        String apSsid = getUniqueAPSSID();
        logInfo("WiFi connect failed. Starting AP: " + apSsid);
        wifiManager.startAP(apSsid.c_str());
        logInfo("AP mode started. Awaiting configuration.");
        Serial.println("[WIFI] Started AP mode: " + apSsid);
    } else {
        setLedState(LED_ON); // Connected!
        logInfo("WiFi connected!");
        logInfo("Local IP: " + WiFi.localIP().toString());
        Serial.println("[WIFI] Connected. IP: " + WiFi.localIP().toString());
    }

    Serial.println("[WEB] Starting web server...");
    webServer.begin(configManager, sensor);
    Serial.println("[WEB] Web server started.");

    if (wifiOk) {
        Serial.println("[MQTT] Connecting to MQTT...");
        if (mqttClient.connect(config)) {
            Serial.println("[MQTT] Connected to MQTT broker.");
        } else {
            Serial.println("[MQTT] MQTT connection failed (will retry in loop).");
        }
    } else {
        Serial.println("[MQTT] Skipping MQTT setup (WiFi not connected).");
    }

    Serial.println("[DISPLAY] Initializing display...");
    // Set hardware type from config
    uint8_t hwType = MD_MAX72XX::FC16_HW;
    if (config.displayHardwareType == "GENERIC_HW") hwType = MD_MAX72XX::GENERIC_HW;
    else if (config.displayHardwareType == "PAROLA_HW") hwType = MD_MAX72XX::PAROLA_HW;
    else if (config.displayHardwareType == "ICSTATION_HW") hwType = MD_MAX72XX::ICSTATION_HW;
    display.setHardwareType(hwType);
    display.begin();
    display.setBrightness(config.displayBrightness);
    Serial.println("[DISPLAY] Display initialized.");
}

unsigned long lastPublish = 0;
const unsigned long publishInterval = 10000; // 10 seconds

void loop() {
    handleLed();
    if (WiFi.status() == WL_CONNECTED) {
        mqttClient.loop();
    } else {
        static unsigned long lastNoMqttLog = 0;
        unsigned long now = millis();
        if (now - lastNoMqttLog > 10000) {
            Serial.println("[MQTT] Skipped: WiFi not connected.");
            lastNoMqttLog = now;
        }
    }

    // Show live water level in selected unit, but only every sensorReadInterval seconds
    static unsigned long lastSensorRead = 0;
    unsigned long now = millis();
    configManager.load(config);
    sensor.setTankHeightCm(config.tankDepth);
    display.setBrightness(config.displayBrightness);
    int intervalMs = (config.sensorReadInterval < 1 ? 1 : config.sensorReadInterval) * 1000;
    if (now - lastSensorRead >= (unsigned long)intervalMs) {
        lastSensorRead = now;
        float percent = sensor.getWaterLevelPercent();
        float distance = sensor.readDistanceCm();
        float tankDepth = config.tankDepth > 0 ? config.tankDepth : 100.0f;
        String displayStr;
        if (distance < 0) {
            displayStr = "Error";
        } else if (config.displayMode == "level") {
            if (config.outputUnit == "cm") {
                float levelCm = tankDepth - distance;
                if (levelCm < 0) levelCm = 0;
                displayStr = String(levelCm, 1) + "cm";
            } else if (config.outputUnit == "in") {
                float levelIn = (tankDepth - distance) / 2.54f;
                if (levelIn < 0) levelIn = 0;
                displayStr = String(levelIn, 1) + "in";
            } else {
                displayStr = String(percent, 1) + "%";
            }
        } else if (config.displayMode == "distance") {
            if (config.outputUnit == "cm") {
                displayStr = String(distance, 1) + "cm";
            } else if (config.outputUnit == "in") {
                displayStr = String(distance/2.54f, 1) + "in";
            } else {
                displayStr = String(distance, 1) + "%";
            }
        } else if (config.displayMode == "percent") {
            displayStr = String(percent, 1) + "%";
        } else if (config.displayMode == "text") {
            displayStr = config.deviceName.length() ? config.deviceName : "WaterLevel";
        } else {
            displayStr = String(percent, 1) + "%";
        }
        // Debug print for display logic
        Serial.printf("[DEBUG] tankDepth: %.1f, distance: %.1f, levelCm: %.1f, levelIn: %.1f, displayStr: %s\n",
            tankDepth, distance, tankDepth - distance, (tankDepth - distance) / 2.54f, displayStr.c_str());
        // Only scroll for text mode, or if not in 'level' mode and value is long
        bool scroll = (config.displayMode == "text") || (config.displayMode != "level" && displayStr.length() > 5);
        static String lastDisplayStr;
        static bool lastScroll = false;
        if (displayStr != lastDisplayStr || scroll != lastScroll) {
            display.displayText(displayStr.c_str(), scroll);
            lastDisplayStr = displayStr;
            lastScroll = scroll;
        }
    }
    display.update();

    // Periodically log sensor connection status
    static unsigned long lastSensorStatus = 0;
    if (now - lastSensorStatus > 15000) {
        lastSensorStatus = now;
        float testDistance = sensor.readDistanceCm();
        if (testDistance >= 0) {
            Serial.println("[SENSOR] Sensor connected. Distance: " + String(testDistance, 1) + " cm");
        } else {
            Serial.println("[SENSOR] Sensor NOT connected! (timeout or error)");
        }
    }
}
