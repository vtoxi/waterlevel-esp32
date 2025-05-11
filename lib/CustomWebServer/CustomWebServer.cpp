#include "CustomWebServer.h"
#include <Arduino.h>
#include <LittleFS.h>
#include "Logger.h"
#include <FS.h>

String settingsNav(const String &active)
{
    String nav = "<nav class='nav'>";
    nav += "<a href='/' class='nav-brand'>Water Level</a>";
    nav += "<button class='nav-toggle' aria-label='Toggle navigation'>";
    nav += "<span></span><span></span><span></span>";
    nav += "</button>";
    nav += "<ul class='nav-list'>";
    nav += "<li><a href='/settings/wifi'" + String(active == "wifi" ? " class='active'" : "") + ">WiFi</a></li>";
    nav += "<li><a href='/settings/mqtt'" + String(active == "mqtt" ? " class='active'" : "") + ">MQTT</a></li>";
    nav += "<li><a href='/settings/tank'" + String(active == "tank" ? " class='active'" : "") + ">Tank</a></li>";
    nav += "<li><a href='/settings/sensor'" + String(active == "sensor" ? " class='active'" : "") + ">Sensor</a></li>";
    nav += "<li><a href='/settings/display'" + String(active == "display" ? " class='active'" : "") + ">Display</a></li>";
    nav += "<li><a href='/settings/network'" + String(active == "network" ? " class='active'" : "") + ">Network</a></li>";
    nav += "<li><a href='/settings/alerts'" + String(active == "alerts" ? " class='active'" : "") + ">Alerts</a></li>";
    nav += "<li><a href='/settings/device'" + String(active == "device" ? " class='active'" : "") + ">Device</a></li>";
    nav += "<li><a href='/logs'" + String(active == "logs" ? " class='active'" : "") + ">Logs</a></li>";
    nav += "<li><a href='/help'" + String(active == "help" ? " class='active'" : "") + ">Help</a></li>";
    nav += "</ul></nav>";
    return nav;
}

// Helper function to get the common HTML head
String getHtmlHead(const String &title) {
    return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)rawliteral" + title + R"rawliteral(</title>
    <link rel="stylesheet" href="/style.css">
    <link rel="stylesheet" href="/logs.css">
    <script src="/script.js"></script>
</head>
<body>
)rawliteral";
}

// Helper function to get the common HTML footer
String getHtmlFooter() {
    return R"rawliteral(
    <footer class="footer">
        <div class="footer-content">
            <div class="footer-section">
                <h4>Water Level Indicator</h4>
                <p>A smart water level monitoring system with real-time alerts and web interface.</p>
            </div>
            <div class="footer-section">
                <h4>Quick Links</h4>
                <ul>
                    <li><a href="/">Home</a></li>
                    <li><a href="/logs">Logs</a></li>
                    <li><a href="/help">Help</a></li>
                </ul>
            </div>
            <div class="footer-section">
                <h4>Connect</h4>
                <ul>
                    <li><a href="https://github.com/vtoxi" target="_blank">GitHub</a></li>
                    <li><a href="mailto:contact@vtoxi.com">Contact</a></li>
                </ul>
            </div>
        </div>
        <div class="footer-bottom">
            <p>Designed and developed by <a href="https://github.com/vtoxi">Faisal S.</a></p>
            <p class="copyright">© 2025 Water Level Indicator. All rights reserved.</p>
        </div>
    </footer>
</body>
</html>
)rawliteral";
}

// Helper function to show WiFi warning
String wifiWarning() {
    return WiFi.isConnected() ? "" : "<div class='wifi-warning'>WiFi is not connected. Some features may not work.</div>";
}

// Helper to load a file from LittleFS as a String
String loadTemplateFile(const String& path) {
    File f = LittleFS.open(path, "r");
    if (!f) return String();
    String content;
    while (f.available()) content += (char)f.read();
    f.close();
    return content;
}

CustomWebServer::CustomWebServer()
    : _server(80)
{
}

void CustomWebServer::begin(ConfigManager &configManager, WaterLevelSensor &sensor)
{
    Logger::info("Initializing web server...");
    LittleFS.begin();
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while(file){
        Logger::info("Found file: " + String(file.name()));
        file = root.openNextFile();
    }
    
    // Initialize event source for logs
    _eventSource = new AsyncEventSource("/logs/stream");
    _eventSource->onConnect([this](AsyncEventSourceClient *client) {
        if (client->lastId()) {
            Logger::info("Client reconnected! Last message ID: " + String(client->lastId()));
        }
        client->send("Connected to log stream", NULL, millis(), 1000);
    });
    _server.addHandler(_eventSource);
    
    setupRoutes(configManager, sensor);
    _server.begin();
    _server.serveStatic("/style.css", LittleFS, "/style.css");
    _server.serveStatic("/script.js", LittleFS, "/script.js");
    _server.serveStatic("/logs.css", LittleFS, "/logs.css");
    _server.serveStatic("/logs.js", LittleFS, "/logs.js");
    _server.serveStatic("/diagram.svg", LittleFS, "/diagram.svg");
    _server.serveStatic("/favicon.png", LittleFS, "/favicon.png");
    Logger::info("Web server started successfully");
}

// Helper function to handle settings updates
void CustomWebServer::setupRoutes(ConfigManager &configManager, WaterLevelSensor &sensor)
{
    // Helper function to handle settings updates
    auto handleSettingsUpdate = [&configManager](AsyncWebServerRequest *request, const String& settingName, std::function<void(Config&)> updateConfig, bool rebootRequired) {
        Logger::info(settingName + " settings update requested");
        Config config;
        configManager.load(config);
        updateConfig(config);
        configManager.save(config);
        Logger::info(settingName + " settings saved" + (rebootRequired ? ", rebooting..." : "."));
        if (rebootRequired) {
            request->send(200, "text/html", "<h2>" + settingName + " Settings Saved! Rebooting...</h2>");
            delay(1000);
            ESP.restart();
        } else {
            request->send(200, "text/html", "<h2>" + settingName + " Settings Saved!</h2><a href='/' class='back-home'>← Back to Home</a>");
        }
    };

    // --- Water Level API Endpoint ---
    _server.on("/api/level", HTTP_GET, [&sensor](AsyncWebServerRequest *request) {
        float percent = sensor.getWaterLevelPercent();
        String json = "{\"level\":" + String(percent, 1) + "}";
        request->send(200, "application/json", json);
    });

    // --- Dashboard with Animated Water Tank ---
    _server.on("/", HTTP_GET, [&configManager, &sensor](AsyncWebServerRequest *request) {
        Logger::info("Home page accessed from IP: " + request->client()->remoteIP().toString());
        Config config;
        configManager.load(config);
        String html = loadTemplateFile("/dashboard.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "Device Home");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        float percent = sensor.getWaterLevelPercent();
        float distance = sensor.readDistanceCm();
        float tankDepth = config.tankDepth > 0 ? config.tankDepth : 100.0f;
        String levelStr;
        if (config.outputUnit == "cm") {
            float levelCm = tankDepth - distance;
            if (levelCm < 0) levelCm = 0;
            levelStr = String(levelCm, 1) + " cm";
        } else if (config.outputUnit == "in") {
            float levelIn = (tankDepth - distance) / 2.54f;
            if (levelIn < 0) levelIn = 0;
            levelStr = String(levelIn, 1) + " in";
        } else {
            levelStr = String(percent, 1) + " %";
        }
        html.replace("{{LEVEL_STR}}", levelStr);
        html.replace("{{OUTPUT_UNIT}}", config.outputUnit);
        html.replace("{{TANK_DEPTH}}", String(tankDepth));
        html.replace("{{DISTANCE}}", String(distance));
        request->send(200, "text/html", html);
    });

    // --- MQTT Settings Page ---
    _server.on("/settings/mqtt", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String html = loadTemplateFile("/settings_mqtt.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "MQTT Setup");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        html.replace("{{MQTT_SERVER}}", config.mqttServer);
        html.replace("{{MQTT_PORT}}", String(config.mqttPort));
        html.replace("{{MQTT_USER}}", config.mqttUser);
        html.replace("{{MQTT_PASSWORD}}", config.mqttPassword);
        html.replace("{{MQTT_TOPIC}}", config.mqttTopic);
        request->send(200, "text/html", html);
    });

    _server.on("/settings/mqtt", HTTP_POST, [&](AsyncWebServerRequest *request)
               {
        handleSettingsUpdate(request, "MQTT", [&](Config& config) {
            config.mqttServer = request->getParam("mqtt", true)->value();
            config.mqttPort = request->getParam("port", true)->value().toInt();
            config.mqttUser = request->getParam("mqttuser", true)->value();
            config.mqttPassword = request->getParam("mqttpass", true)->value();
            config.mqttTopic = request->getParam("mqtttopic", true)->value();
        }, false);
    });

    // --- Tank Settings Page ---
    _server.on("/settings/tank", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String html = loadTemplateFile("/settings_tank.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "Tank Setup");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        float tankDepth = (config.outputUnit == "in" ? config.tankDepth / 2.54f : config.tankDepth);
        html.replace("{{TANK_DEPTH}}", String(tankDepth, 1));
        html.replace("{{TANK_DEPTH_UNIT_CM_SELECTED}}", config.outputUnit == "cm" ? "selected" : "");
        html.replace("{{TANK_DEPTH_UNIT_IN_SELECTED}}", config.outputUnit == "in" ? "selected" : "");
        html.replace("{{OUTPUT_UNIT_CM_SELECTED}}", config.outputUnit == "cm" ? "selected" : "");
        html.replace("{{OUTPUT_UNIT_IN_SELECTED}}", config.outputUnit == "in" ? "selected" : "");
        html.replace("{{OUTPUT_UNIT_PERCENT_SELECTED}}", config.outputUnit == "percent" ? "selected" : "");
        request->send(200, "text/html", html);
    });

    _server.on("/settings/tank", HTTP_POST, [&](AsyncWebServerRequest *request)
               {
        handleSettingsUpdate(request, "Tank", [&](Config& config) {
            float depth = request->getParam("tankDepth", true)->value().toFloat();
            String depthUnit = request->getParam("tankDepthUnit", true)->value();
            if (depthUnit == "in") {
                depth *= 2.54f;
                Logger::info("Converting tank depth from inches to cm: " + String(depth));
            }
            config.tankDepth = depth;
            config.outputUnit = request->getParam("outputUnit", true)->value();
            // No direct hardware update here; main loop will apply changes
        }, false);
    });

    // Add this route in setupRoutes:
    _server.on("/connected", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String ip = WiFi.localIP().toString();
        String html = loadTemplateFile("/connected.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "Connected");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        html.replace("{{IP}}", ip);
        request->send(200, "text/html", html);
    });

    _server.on("/settings/sensor", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String html = loadTemplateFile("/settings_sensor.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "Sensor Calibration");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        html.replace("{{SENSOR_OFFSET}}", String(config.sensorOffset, 1));
        html.replace("{{SENSOR_FULL}}", String(config.sensorFull, 1));
        html.replace("{{SENSOR_READ_INTERVAL}}", String(config.sensorReadInterval));
        request->send(200, "text/html", html);
    });

    _server.on("/settings/sensor", HTTP_POST, [&](AsyncWebServerRequest *request)
               {
        handleSettingsUpdate(request, "Sensor", [&](Config& config) {
            config.sensorOffset = request->getParam("offset", true)->value().toFloat();
            config.sensorFull = request->getParam("full", true)->value().toFloat();
            int interval = request->getParam("sensorReadInterval", true)->value().toInt();
            config.sensorReadInterval = interval < 1 ? 1 : interval;
            // No direct hardware update here; main loop will apply changes
        }, false);
    });

    _server.on("/settings/display", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String html = loadTemplateFile("/settings_display.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "Display Settings");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        html.replace("{{BRIGHTNESS}}", String(config.displayBrightness));
        html.replace("{{LEVEL_SELECTED}}", config.displayMode == "level" ? "selected" : "");
        html.replace("{{PERCENT_SELECTED}}", config.displayMode == "percent" ? "selected" : "");
        html.replace("{{DISTANCE_SELECTED}}", config.displayMode == "distance" ? "selected" : "");
        html.replace("{{TEXT_SELECTED}}", config.displayMode == "text" ? "selected" : "");
        html.replace("{{FC16_SELECTED}}", config.displayHardwareType == "FC16_HW" ? "selected" : "");
        html.replace("{{GENERIC_SELECTED}}", config.displayHardwareType == "GENERIC_HW" ? "selected" : "");
        html.replace("{{PAROLA_SELECTED}}", config.displayHardwareType == "PAROLA_HW" ? "selected" : "");
        html.replace("{{ICSTATION_SELECTED}}", config.displayHardwareType == "ICSTATION_HW" ? "selected" : "");
        html.replace("{{SCROLL_CHECKED}}", config.displayScrollEnabled ? "checked" : "");
        request->send(200, "text/html", html);
    });

    _server.on("/settings/display", HTTP_POST, [&](AsyncWebServerRequest *request)
               {
        handleSettingsUpdate(request, "Display", [&](Config& config) {
            config.displayBrightness = request->getParam("brightness", true)->value().toInt();
            config.displayMode = request->getParam("mode", true)->value();
            config.displayHardwareType = request->getParam("hardwareType", true)->value();
            config.displayScrollEnabled = request->hasParam("scroll", true);
            Serial.printf("[DEBUG] Saving displayScrollEnabled: %d\n", config.displayScrollEnabled);
            // No direct hardware update here; main loop will apply changes
        }, false);
    });

    _server.on("/settings/network", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String html = loadTemplateFile("/settings_network.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "Network Settings");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        html.replace("{{STATIC_IP}}", config.staticIp);
        html.replace("{{GATEWAY}}", config.gateway);
        html.replace("{{SUBNET}}", config.subnet);
        html.replace("{{HOSTNAME}}", config.hostname);
        request->send(200, "text/html", html);
    });

    _server.on("/settings/network", HTTP_POST, [&](AsyncWebServerRequest *request){
        handleSettingsUpdate(request, "Network", [&](Config& config) {
            config.staticIp = request->getParam("staticip", true)->value();
            config.gateway = request->getParam("gateway", true)->value();
            config.subnet = request->getParam("subnet", true)->value();
            config.hostname = request->getParam("hostname", true)->value();
            // Network changes require reboot
        }, true);
    });

    _server.on("/settings/alerts", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String html = loadTemplateFile("/settings_alerts.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "Alert Settings");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        html.replace("{{ALERT_LOW}}", String(config.alertLow));
        html.replace("{{ALERT_HIGH}}", String(config.alertHigh));
        html.replace("{{ALERT_METHOD_MQTT_SELECTED}}", config.alertMethod == "mqtt" ? "selected" : "");
        html.replace("{{ALERT_METHOD_BUZZER_SELECTED}}", config.alertMethod == "buzzer" ? "selected" : "");
        html.replace("{{ALERT_METHOD_LED_SELECTED}}", config.alertMethod == "led" ? "selected" : "");
        request->send(200, "text/html", html);
    });

    _server.on("/settings/alerts", HTTP_POST, [&](AsyncWebServerRequest *request){
        handleSettingsUpdate(request, "Alert", [&](Config& config) {
            config.alertLow = request->getParam("low", true)->value().toInt();
            config.alertHigh = request->getParam("high", true)->value().toInt();
            config.alertMethod = request->getParam("alertmethod", true)->value();
            // No direct hardware update here; main loop will apply changes
        }, false);
    });

    _server.on("/settings/device", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String html = loadTemplateFile("/settings_device.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "Device Info / Reset");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        html.replace("{{DEVICE_NAME}}", config.deviceName);
        html.replace("{{OTA_ON_SELECTED}}", config.otaEnabled == "on" ? "selected" : "");
        html.replace("{{OTA_OFF_SELECTED}}", config.otaEnabled == "off" ? "selected" : "");
        request->send(200, "text/html", html);
    });

    _server.on("/settings/device", HTTP_POST, [&](AsyncWebServerRequest *request)
               {
        handleSettingsUpdate(request, "Device", [&](Config& config) {
            config.deviceName = request->getParam("devicename", true)->value();
            config.otaEnabled = request->getParam("ota", true)->value();
            // No direct hardware update here; main loop will apply changes
        }, false);
    });

    _server.on("/settings/device/reset", HTTP_POST, [&](AsyncWebServerRequest *request) {
        Logger::info("Factory reset requested");
        Config config;
        configManager.load(config);
        // Reset all values to defaults
        config.wifiSsid = "";
        config.wifiPassword = "";
        config.mqttServer = "";
        config.mqttPort = 1883;
        config.mqttUser = "";
        config.mqttPassword = "";
        config.mqttTopic = "home/waterlevel";
        config.tankDepth = 100.0f;
        config.outputUnit = "cm";
        config.sensorOffset = 0.0f;
        config.sensorFull = 0.0f;
        config.displayBrightness = 8;
        config.displayMode = "level";
        config.staticIp = "";
        config.gateway = "";
        config.subnet = "";
        config.hostname = "";
        config.alertLow = 0;
        config.alertHigh = 100;
        config.alertMethod = "mqtt";
        config.deviceName = "";
        config.otaEnabled = "off";
        Logger::info("Resetting all settings to defaults");
        if (configManager.save(config)) {
            configManager.reset();
            Logger::info("Factory reset completed successfully");
            String html = loadTemplateFile("/reset_success.html");
            String header = loadTemplateFile("/header.html");
            String footer = loadTemplateFile("/footer.html");
            header.replace("{{TITLE}}", "Factory Reset Complete");
            html.replace("{{HEADER}}", header);
            html.replace("{{FOOTER}}", footer);
            request->send(200, "text/html", html);
            delay(2000);
            ESP.restart();
        } else {
            Logger::error("Factory reset failed!");
            String html = loadTemplateFile("/reset_failed.html");
            String header = loadTemplateFile("/header.html");
            String footer = loadTemplateFile("/footer.html");
            header.replace("{{TITLE}}", "Reset Failed");
            html.replace("{{HEADER}}", header);
            html.replace("{{FOOTER}}", footer);
            request->send(500, "text/html", html);
        }
    });

    // --- WiFi Settings Page ---
    _server.on("/settings/wifi", HTTP_GET, [&](AsyncWebServerRequest *request){
        Config config;
        configManager.load(config);
        String html = loadTemplateFile("/settings_wifi.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "WiFi Setup");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        html.replace("{{WIFI_SSID}}", config.wifiSsid);
        html.replace("{{WIFI_PASSWORD}}", config.wifiPassword);
        request->send(200, "text/html", html);
    });

    // WiFi POST handler for AJAX
    _server.on("/settings/wifi", HTTP_POST, [&](AsyncWebServerRequest *request) {
        String debugMsg;
        if (!request->hasParam("ssid", true) || !request->hasParam("wifipass", true)) {
            debugMsg = "[DEBUG] Missing ssid or wifipass in POST";
            Serial.println(debugMsg);
            request->send(400, "text/html", "<span style='color:red;'>Missing required WiFi parameters.</span>");
            return;
        }
        Config config;
        if (!configManager.load(config)) {
            debugMsg = "[DEBUG] Failed to load config in WiFi POST";
            Serial.println(debugMsg);
            request->send(500, "text/html", "<span style='color:red;'>Failed to load configuration.</span>");
            return;
        }
        String ssid = request->getParam("ssid", true)->value();
        String wifipass = request->getParam("wifipass", true)->value();
        debugMsg = "[DEBUG] Received ssid: " + ssid + ", wifipass: " + wifipass;
        Serial.println(debugMsg);
        config.wifiSsid = ssid;
        config.wifiPassword = wifipass;
        if (!configManager.save(config)) {
            debugMsg = "[DEBUG] Failed to save config in WiFi POST";
            Serial.println(debugMsg);
            request->send(500, "text/html", "<span style='color:red;'>Failed to save configuration.</span>");
            return;
        }
        debugMsg = "[DEBUG] WiFi settings saved, rebooting...";
        Serial.println(debugMsg);
        request->send(200, "text/html", "<span style='color:green;'>WiFi settings saved! Rebooting...</span>");
        delay(1000);
        ESP.restart();
    });

    // WiFi scan endpoint
    _server.on("/scan/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        int n = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < n; ++i) {
            if (i > 0) json += ",";
            json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        }
        json += "]";
        request->send(200, "application/json", json);
    });

    // --- Logs Page ---
    _server.on("/logs", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String html = loadTemplateFile("/logs.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "Device Logs");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        request->send(200, "text/html", html);
    });

    // Serve the log file for download or viewing
    _server.on("/logs/file", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/logs.txt", "text/plain");
    });

    // Add clear logs endpoint
    _server.on("/logs/clear", HTTP_POST, [&](AsyncWebServerRequest *request) {
        Logger::clearLogs();
        request->send(200);
    });

    // --- Help Page ---
    _server.on("/help", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String html = loadTemplateFile("/help.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "Connection Help");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        request->send(200, "text/html", html);
    });

    // --- API endpoint for live brightness change ---
    _server.on("/api/display/brightness", HTTP_POST, [&](AsyncWebServerRequest *request){
        if (request->hasParam("value", true)) {
            int value = request->getParam("value", true)->value().toInt();
            value = std::max(0, std::min(15, value));
            Config config;
            configManager.load(config);
            config.displayBrightness = value;
            configManager.save(config);
            request->send(200, "application/json", "{}\n");
        } else {
            request->send(400, "application/json", "{\"error\":\"Missing value\"}\n");
        }
    });

    // 404 Not Found handler (must be last)
    _server.onNotFound([](AsyncWebServerRequest *request) {
        String html = loadTemplateFile("/404.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        header.replace("{{TITLE}}", "404 - Page Not Found");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        request->send(404, "text/html", html);
    });
}

void CustomWebServer::handleClient()
{
    // Not needed for AsyncWebServer, but present for interface compatibility
}

