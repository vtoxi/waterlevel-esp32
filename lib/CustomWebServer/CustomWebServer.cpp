#include "CustomWebServer.h"
#include <Arduino.h>
#include <LittleFS.h>
#include "Logger.h"
#include <FS.h>

extern float lastDistance;

// Place getDisplayString at file scope, before any other code
String getDisplayString(const Config& config, float distance, float& percentOut) {
    float tankDepth = config.tankDepth > 0 ? config.tankDepth : 100.0f;
    percentOut = (distance < 0 || tankDepth <= 0) ? 0.0f : ((tankDepth - distance) / tankDepth * 100.0f);
    if (distance < 0) return "ERROR";
    if (distance > tankDepth) return "RANGE ERR" + String(distance);
    if (config.displayMode == "level") {
        if (config.outputUnit == "cm") {
            float levelCm = tankDepth - distance;
            if (levelCm < 0) levelCm = 0;
            return String(levelCm, 1) + " cm";
        } else if (config.outputUnit == "in") {
            float levelIn = (tankDepth - distance) / 2.54f;
            if (levelIn < 0) levelIn = 0;
            return String(levelIn, 1) + " in";
        } else {
            return String(percentOut, 1) + " %";
        }
    } else if (config.displayMode == "distance") {
        if (config.outputUnit == "cm") {
            return String(distance, 1) + " cm";
        } else if (config.outputUnit == "in") {
            return String(distance/2.54f, 1) + " in";
        } else {
            return String(distance, 1) + " %";
        }
    } else if (config.displayMode == "percent") {
        return String(percentOut, 1) + "%";
    } else if (config.displayMode == "volume") {
        // Volume calculation
        float levelCm = tankDepth - distance;
        if (levelCm < 0) levelCm = 0;
        float volumeL = 0.0f;
        if (config.tankShape == "cylinder" && config.tankDiameter > 0) {
            float radius = config.tankDiameter / 2.0f;
            float area = 3.14159265f * radius * radius;
            volumeL = area * levelCm / 1000.0f; // cm^2 * cm = cm^3, /1000 = L
        } else if (config.tankShape == "rectangle" && config.tankWidth > 0 && config.tankLength > 0) {
            float area = config.tankWidth * config.tankLength;
            volumeL = area * levelCm / 1000.0f;
        } else {
            return "N/A";
        }
        if (config.outputUnit == "gal") {
            float volumeGal = volumeL * 0.264172f;
            return String(volumeGal, 1) + " gal";
        } else {
            return String(volumeL, 1) + " L";
        }
    } else if (config.displayMode == "text") {
        return config.deviceName.length() ? config.deviceName : "WaterLevel";
    } else if (config.displayMode == "status") {
        if (distance < 0) return "ERROR";
        if (percentOut < config.alertLow) return "LOW";
        if (percentOut >= config.alertHigh) return "FULL";
        return "OK";
    } else {
        return String(percentOut, 1) + "%";
    }
}

extern volatile bool shouldReboot;

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
            shouldReboot = true;
        } else {
            request->send(200, "text/html", "<h2>" + settingName + " Settings Saved!</h2>");
        }
    };

    // --- Water Level API Endpoint ---
    _server.on("/api/level", HTTP_GET, [&sensor, &configManager](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        sensor.setTankHeightCm(config.tankDepth);
        float percent = 0.0f;
        String displayStr = getDisplayString(config, lastDistance, percent);
        String json = "{\"level\":" + String(percent, 1) + ",\"display\":\"" + displayStr + "\"}";
        request->send(200, "application/json", json);
    });

    // --- Volume Unit API Endpoint ---
    _server.on("/api/volumeunit", HTTP_GET, [&configManager](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String unit = config.volumeUnit.length() ? config.volumeUnit : "L";
        request->send(200, "application/json", "{\"unit\":\"" + unit + "\"}");
    });

    _server.on("/api/volumeunit", HTTP_POST, [&configManager](AsyncWebServerRequest *request) {
        if (request->contentType() == "application/json") {
            String body;
            if (request->hasParam("plain", true)) {
                body = request->getParam("plain", true)->value();
            }
            int idx = body.indexOf("unit");
            if (idx != -1) {
                int colon = body.indexOf(":", idx);
                int quote1 = body.indexOf('"', colon);
                int quote2 = body.indexOf('"', quote1 + 1);
                String unit = body.substring(quote1 + 1, quote2);
                if (unit == "L" || unit == "gal") {
                    Config config;
                    configManager.load(config);
                    config.volumeUnit = unit;
                    configManager.save(config);
                    request->send(200, "application/json", "{}\n");
                    return;
                }
            }
        }
        request->send(400, "application/json", "{\"error\":\"Invalid unit\"}\n");
    });

    // --- Dashboard with Animated Water Tank ---
    _server.on("/", HTTP_GET, [&configManager, &sensor](AsyncWebServerRequest *request) {
        Logger::info("Home page accessed from IP: " + request->client()->remoteIP().toString());
        Config config;
        configManager.load(config);
        sensor.setTankHeightCm(config.tankDepth);
        String html = loadTemplateFile("/dashboard.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        html.replace("{{TITLE}}", "Device Home");
        header.replace("{{TITLE}}", "Device Home");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        float percent = 0.0f;
        String levelStr = getDisplayString(config, lastDistance, percent);
        String tankIconClass = (levelStr == "ERROR" || levelStr.startsWith("RANGE ERR")) ? "tank-error" : "";
        html.replace("{{LEVEL_STR}}", levelStr);
        html.replace("{{TANK_ICON_CLASS}}", tankIconClass);
        html.replace("{{OUTPUT_UNIT}}", config.outputUnit);
        html.replace("{{TANK_DEPTH}}", String(config.tankDepth));
        html.replace("{{TANK_WIDTH}}", String(config.tankWidth));
        html.replace("{{TANK_LENGTH}}", String(config.tankLength));
        html.replace("{{TANK_DIAMETER}}", String(config.tankDiameter));
        html.replace("{{TANK_SHAPE}}", config.tankShape);
        html.replace("{{RECT_STYLE}}", config.tankShape == "rectangle" ? "display:block;" : "display:none;");
        html.replace("{{CYL_STYLE}}", config.tankShape == "cylinder" ? "display:block;" : "display:none;");
        html.replace("{{DISTANCE}}", String(lastDistance));
        String displayModeForDashboard = config.outputUnit == "quantity" ? "volume" : config.displayMode;
        html.replace("{{DISPLAY_MODE}}", displayModeForDashboard);
        html.replace("{{VOLUME_UNIT}}", config.volumeUnit.length() ? config.volumeUnit : "L");
        request->send(200, "text/html", html);
    });

    // --- MQTT Settings Page ---
    _server.on("/settings/mqtt", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String html = loadTemplateFile("/settings_mqtt.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        html.replace("{{TITLE}}", "MQTT Setup");
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
        html.replace("{{TITLE}}", "Tank Setup");
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
        html.replace("{{OUTPUT_UNIT_QUANTITY_SELECTED}}", config.outputUnit == "quantity" ? "selected" : "");
        html.replace("{{TANK_SHAPE_RECTANGLE_SELECTED}}", config.tankShape == "rectangle" ? "selected" : "");
        html.replace("{{TANK_SHAPE_CYLINDER_SELECTED}}", config.tankShape == "cylinder" ? "selected" : "");
        html.replace("{{TANK_WIDTH}}", String(config.tankWidth, 1));
        html.replace("{{TANK_LENGTH}}", String(config.tankLength, 1));
        html.replace("{{TANK_DIAMETER}}", String(config.tankDiameter, 1));
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
            if (request->hasParam("outputUnit", true)) {
                config.outputUnit = request->getParam("outputUnit", true)->value();
            }
            // New fields
            config.tankShape = request->getParam("tankShape", true)->value();
            config.tankWidth = request->hasParam("tankWidth", true) ? request->getParam("tankWidth", true)->value().toFloat() : 0.0f;
            config.tankLength = request->hasParam("tankLength", true) ? request->getParam("tankLength", true)->value().toFloat() : 0.0f;
            config.tankDiameter = request->hasParam("tankDiameter", true) ? request->getParam("tankDiameter", true)->value().toFloat() : 0.0f;
            // No direct hardware update here; main loop will apply changes
        }, false);
    });

    // Add this route in setupRoutes:
    _server.on("/connected", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String ip = WiFi.localIP().toString();
        String html = loadTemplateFile("/connected.html");
        String header = loadTemplateFile("/header.html");
        String footer = loadTemplateFile("/footer.html");
        html.replace("{{TITLE}}", "Connected");
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
        html.replace("{{TITLE}}", "Sensor Calibration");
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
        html.replace("{{TITLE}}", "Display Settings");
        header.replace("{{TITLE}}", "Display Settings");
        html.replace("{{HEADER}}", header);
        html.replace("{{FOOTER}}", footer);
        html.replace("{{BRIGHTNESS}}", String(config.displayBrightness));
        html.replace("{{LEVEL_SELECTED}}", config.displayMode == "level" ? "selected" : "");
        html.replace("{{PERCENT_SELECTED}}", config.displayMode == "percent" ? "selected" : "");
        html.replace("{{DISTANCE_SELECTED}}", config.displayMode == "distance" ? "selected" : "");
        html.replace("{{VOLUME_SELECTED}}", config.displayMode == "volume" ? "selected" : "");
        html.replace("{{STATUS_SELECTED}}", config.displayMode == "status" ? "selected" : "");
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
        html.replace("{{TITLE}}", "Network Settings");
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
        html.replace("{{TITLE}}", "Alert Settings");
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
        html.replace("{{TITLE}}", "Device Info / Reset");
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
            html.replace("{{TITLE}}", "Factory Reset Complete");
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
            html.replace("{{TITLE}}", "Reset Failed");
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
        html.replace("{{TITLE}}", "WiFi Setup");
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
        html.replace("{{TITLE}}", "Device Logs");
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
        html.replace("{{TITLE}}", "Connection Help");
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
        html.replace("{{TITLE}}", "404 - Page Not Found");
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

