#include <Arduino.h>
#include "WaterLevelSensor.h"
#include "ConfigManager.h"
#include "WiFiManager.h"
#include "MQTTClient.h"
#include "CustomWebServer.h"
#include "DisplayManager.h"
#include "Logger.h"
#include <LittleFS.h>
#include "LogManager.h"
#include "SevenSegmentDisplayManager.h"
#include "IDisplayManager.h"
#include "SSD1306DisplayManager.h"

// Pin definitions (adjust as needed)
constexpr int TRIGGER_PIN = 17;
constexpr int ECHO_PIN = 5;
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
#define RESET_BUTTON_PIN 0  // GPIO for hard reset button

enum LedState { LED_OFF, LED_ON, LED_BLINK_SLOW, LED_BLINK_FAST };
LedState ledState = LED_OFF;
unsigned long lastLedToggle = 0;
const unsigned long BLINK_INTERVAL_SLOW = 500;  // ms
const unsigned long BLINK_INTERVAL_FAST = 150;  // ms
bool ledOn = false;

volatile bool shouldReboot = false;

float lastDistance = -1.0f;

SevenSegmentDisplayManager sevenSegmentDisplay(DATA_PIN, CLK_PIN, CS_PIN);

SSD1306DisplayManager ssd1306Display(128, 64, 21, 22); // default pins, will re-init if needed

IDisplayManager* displayPtr = nullptr;

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

// Forward declaration for getDisplayString
String getDisplayString(const Config& config, float distance, float& percentOut);

void setup() {
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
    // Check for hard reset button held at boot
    if (digitalRead(RESET_BUTTON_PIN) == LOW) {
        unsigned long start = millis();
        while (digitalRead(RESET_BUTTON_PIN) == LOW) {
            if (millis() - start > 3000) { // 3 seconds
                Serial.println("[RESET] Hard reset button held. Resetting all settings to default...");
                ConfigManager configManager;
                configManager.reset();
                WiFi.disconnect(true, true);
                delay(1000);
                ESP.restart();
            }
            delay(10);
        }
    }
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

    // --- DHCP fallback logic ---
    bool staticOk = false;
    if (config.staticIp.length() > 0 && config.gateway.length() > 0 && config.subnet.length() > 0) {
        IPAddress ip, gw, sn;
        if (ip.fromString(config.staticIp) && gw.fromString(config.gateway) && sn.fromString(config.subnet)) {
            // Check if all are in the same subnet (simple check: first 3 octets match)
            if ((ip[0] == gw[0] && ip[1] == gw[1] && ip[2] == gw[2]) &&
                (ip[0] == sn[0] && ip[1] == sn[1] && ip[2] == sn[2])) {
                WiFi.config(ip, gw, sn);
                staticOk = true;
                Serial.printf("[WIFI] Using static IP: %s\n", config.staticIp.c_str());
            }
        }
    }
    if (!staticOk) {
        WiFi.config(0U, 0U, 0U); // Force DHCP
        Serial.println("[WIFI] Using DHCP (no valid static IP config)");
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
    if (config.displayType == "ssd1306") {
        // Re-init with correct size and pins
        new (&ssd1306Display) SSD1306DisplayManager(config.ssd1306Width, config.ssd1306Height, 21, 22); // SDA=21, SCL=22 (adjust if needed)
        ssd1306Display.begin();
        displayPtr = &ssd1306Display;
        Serial.println("[DISPLAY] SSD1306DisplayManager initialized.");
    } else if (config.displayType == "sevensegment") {
        sevenSegmentDisplay.begin();
        displayPtr = &sevenSegmentDisplay;
        Serial.println("[DISPLAY] SevenSegmentDisplayManager initialized.");
    } else {
        uint8_t hwType = MD_MAX72XX::FC16_HW;
        if (config.displayHardwareType == "GENERIC_HW") hwType = MD_MAX72XX::GENERIC_HW;
        else if (config.displayHardwareType == "PAROLA_HW") hwType = MD_MAX72XX::PAROLA_HW;
        else if (config.displayHardwareType == "ICSTATION_HW") hwType = MD_MAX72XX::ICSTATION_HW;
        display.setHardwareType(hwType);
        display.begin();
        display.setBrightness(config.displayBrightness);
        displayPtr = &display;
        Serial.println("[DISPLAY] Matrix DisplayManager initialized.");
    }

    LittleFS.begin();
    LogManager::initLogFile();
    // Create CSV file with header if it doesn't exist
    if (!LittleFS.exists("/level_log.csv")) {
        File f = LittleFS.open("/level_log.csv", "w");
        if (f) {
            f.println("timestamp,distance_cm,percent,level_cm,level_in,liters,gallons");
            f.close();
        }
    }
}

unsigned long lastPublish = 0;
const unsigned long publishInterval = 10000; // 10 seconds

void loop() {
    handleLed();
    if (shouldReboot) {
        delay(500); // Allow time for HTTP response to flush
        ESP.restart();
    }
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
    float percent = 0.0f;
    String displayStr = getDisplayString(config, lastDistance, percent);
    if (config.displayType == "sevensegment") {
        static float lastValue = NAN;
        float value = 0.0f;
        if (config.displayMode == "level" || config.displayMode == "volume") {
            value = config.tankDepth - lastDistance;
        } else if (config.displayMode == "distance") {
            value = lastDistance;
        } else if (config.displayMode == "percent") {
            value = percent;
        } else {
            value = displayStr.toFloat();
        }
        if (value != lastValue) {
            displayPtr->displayNumber(value);
            lastValue = value;
        }
    } else {
        bool scroll = config.displayScrollEnabled;
        static String lastDisplayStr;
        static bool lastScroll = false;
        if (config.displayScrollEnabled != lastScroll) {
            lastDisplayStr = "";
            lastScroll = config.displayScrollEnabled;
        }
        if (displayStr != lastDisplayStr || scroll != lastScroll) {
            displayPtr->displayText(displayStr.c_str(), scroll);
            lastDisplayStr = displayStr;
            lastScroll = scroll;
        }
        if (config.displayType == "matrix") display.update();
    }

    // MQTT publish (every publishInterval)
    static unsigned long lastPublish = 0;
    if (now - lastPublish > publishInterval) {
        lastPublish = now;
        if (mqttClient.isConnected()) {
            String payload = "{\"display\":\"" + displayStr + "\",\"percent\":" + String(percent, 1) + ",\"distance\":" + String(lastDistance, 1) + "}";
            mqttClient.publish(config.mqttTopic, payload);
        }
    }

    // Periodically log sensor connection status
    static unsigned long lastSensorStatus = 0;
    if (now - lastSensorStatus > 15000) {
        lastSensorStatus = now;
        if (lastDistance >= 0) {
            Serial.println("[SENSOR] Sensor connected. Display: " + displayStr);
        } else {
            Serial.println("[SENSOR] Sensor NOT connected! (timeout or error)");
        }
    }

    // Periodic water level logging
    static unsigned long lastLog = 0;
    const unsigned long logInterval = 60000; // 60 seconds
    if (now - lastLog > logInterval) {
        lastLog = now;
        float tankDepth = config.tankDepth;
        float distance = lastDistance;
        float levelCm = tankDepth - distance;
        if (levelCm < 0) levelCm = 0;
        float levelIn = levelCm / 2.54f;
        float liters = 0.0f;
        float gallons = 0.0f;
        if (config.tankShape == "rectangle" && config.tankWidth > 0 && config.tankLength > 0) {
            liters = (config.tankWidth * config.tankLength * levelCm) / 1000.0f;
        } else if (config.tankShape == "cylinder" && config.tankDiameter > 0) {
            float radius = config.tankDiameter / 2.0f;
            float area = 3.14159265f * radius * radius;
            liters = (area * levelCm) / 1000.0f;
        }
        gallons = liters * 0.264172f;
        float percent = (distance < 0 || tankDepth <= 0) ? 0.0f : ((tankDepth - distance) / tankDepth * 100.0f);
        if (percent < 0) percent = 0;
        LogManager::logLevelReading(millis()/1000UL, distance, percent, levelCm, levelIn, liters, gallons);
    }
}
