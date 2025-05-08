#include "CustomWebServer.h"
#include <Arduino.h>
#include <LittleFS.h>
#include "Logger.h"
#include <ESPAsyncWebServer.h>

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

AsyncWebServer server(80);

CustomWebServer::CustomWebServer() : server(80) {}

void CustomWebServer::begin(ConfigManager& configManager, WaterLevelSensor& sensor) {
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount file system");
        return;
    }

    // Serve static files with read mode
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // API endpoints
    server.on("/api/config/wifi", HTTP_POST, [&configManager](AsyncWebServerRequest* request) {
        Config config;
        configManager.load(config);
        
        if (request->hasParam("ssid", true) && request->hasParam("wifipass", true)) {
            strncpy(config.wifiSSID, request->getParam("ssid", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            strncpy(config.wifiPassword, request->getParam("wifipass", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            configManager.save(config);
            request->send(200, "text/plain", "WiFi settings updated");
        } else {
            request->send(400, "text/plain", "Missing parameters");
        }
    });

    server.on("/api/config/mqtt", HTTP_POST, [&configManager](AsyncWebServerRequest* request) {
        Config config;
        configManager.load(config);
        
        if (request->hasParam("mqtt", true)) {
            strncpy(config.mqttServer, request->getParam("mqtt", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            config.mqttPort = request->hasParam("mqttport", true) ? 
                request->getParam("mqttport", true)->value().toInt() : 1883;
            
            if (request->hasParam("mqttuser", true)) {
                strncpy(config.mqttUser, request->getParam("mqttuser", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            }
            if (request->hasParam("mqttpass", true)) {
                strncpy(config.mqttPassword, request->getParam("mqttpass", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            }
            if (request->hasParam("mqtttopic", true)) {
                strncpy(config.mqttTopic, request->getParam("mqtttopic", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            }
            
            configManager.save(config);
            request->send(200, "text/plain", "MQTT settings updated");
        } else {
            request->send(400, "text/plain", "Missing parameters");
        }
    });

    server.begin();
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
    server.on("/api/level", HTTP_GET, [&](AsyncWebServerRequest *request) {
        float percent = sensor.getWaterLevelPercent();
        String json = "{\"level\":" + String(percent, 1) + "}";
        request->send(200, "application/json", json);
    });

    // --- Dashboard with Animated Water Tank ---
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Logger::info("Home page accessed from IP: " + request->client()->remoteIP().toString());
        String html = getHtmlHead("Device Home");
        html += settingsNav("home");
        html += R"rawliteral(
  <div class="container">
    <h2>Device Dashboard</h2>
    <div class="water-tank-section">
      <h3>Real-Time Water Level</h3>
      <div class="tank-visual">
        <svg id="tank-svg" viewBox="0 0 120 240" width="120" height="240">
          <rect x="20" y="20" width="80" height="200" rx="30" fill="#e0e0e0" stroke="#2196f3" stroke-width="4"/>
          <rect id="water-fill" x="20" y="220" width="80" height="0" rx="30" fill="#21cbf3"/>
          <text id="level-text" x="60" y="130" text-anchor="middle" font-size="32" fill="#1976d2" font-weight="bold">--%</text>
        </svg>
      </div>
    </div>
    <div class="dashboard-grid">
      <a href="/settings/wifi" class="dashboard-widget">
        <span class="widget-icon">🔗</span>
        <span class="widget-label">WiFi</span>
      </a>
      <a href="/settings/mqtt" class="dashboard-widget">
        <span class="widget-icon">☁️</span>
        <span class="widget-label">MQTT</span>
      </a>
      <a href="/settings/tank" class="dashboard-widget">
        <span class="widget-icon">🛢️</span>
        <span class="widget-label">Tank</span>
      </a>
      <a href="/settings/sensor" class="dashboard-widget">
        <span class="widget-icon">📡</span>
        <span class="widget-label">Sensor</span>
      </a>
      <a href="/settings/display" class="dashboard-widget">
        <span class="widget-icon">💡</span>
        <span class="widget-label">Display</span>
      </a>
      <a href="/settings/network" class="dashboard-widget">
        <span class="widget-icon">🌐</span>
        <span class="widget-label">Network</span>
      </a>
      <a href="/settings/alerts" class="dashboard-widget">
        <span class="widget-icon">🔔</span>
        <span class="widget-label">Alerts</span>
      </a>
      <a href="/settings/device" class="dashboard-widget">
        <span class="widget-icon">⚙️</span>
        <span class="widget-label">Device</span>
      </a>
      <a href="/logs" class="dashboard-widget">
        <span class="widget-icon">📝</span>
        <span class="widget-label">Logs</span>
      </a>
      <a href="/help" class="dashboard-widget">
        <span class="widget-icon">❓</span>
        <span class="widget-label">Help</span>
      </a>
    </div>
  </div>
  <script>
    let currentLevel = 0;
    function animateTank(target) {
      const fill = document.getElementById('water-fill');
      const text = document.getElementById('level-text');
      let displayed = currentLevel;
      function step() {
        displayed += (target - displayed) * 0.15;
        if (Math.abs(displayed - target) < 0.2) displayed = target;
        let h = Math.max(0, Math.min(200, 2 * displayed));
        fill.setAttribute('y', 220 - h);
        fill.setAttribute('height', h);
        text.textContent = Math.round(displayed) + '%';
        if (displayed !== target) requestAnimationFrame(step);
        else currentLevel = target;
      }
      step();
    }
    function pollLevel() {
      fetch('/api/level').then(r => r.json()).then(data => {
        animateTank(data.level);
      });
    }
    setInterval(pollLevel, 1200);
    window.addEventListener('DOMContentLoaded', pollLevel);
  </script>
  <style>
    .water-tank-section {
      display: flex;
      flex-direction: column;
      align-items: center;
      margin-bottom: 32px;
    }
    .tank-visual {
      margin: 0 auto;
      background: #f5fafd;
      border-radius: 16px;
      box-shadow: 0 2px 12px rgba(33,150,243,0.08);
      padding: 16px 24px 8px 24px;
      max-width: 180px;
    }
    #tank-svg {
      display: block;
      margin: 0 auto;
    }
  </style>
)rawliteral";
        html += getHtmlFooter();
        request->send(200, "text/html", html);
    });

    server.on("/", HTTP_POST, [&](AsyncWebServerRequest *request)
               {
        handleSettingsUpdate(request, "WiFi", [&](Config& config) {
            strncpy(config.wifiSSID, request->getParam("ssid", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            strncpy(config.wifiPassword, request->getParam("wifipass", true)->value().c_str(), MAX_STRING_LENGTH - 1);
        }, true);
    });

    // --- MQTT Setup Page ---
    server.on("/settings/mqtt", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        
        String html = getHtmlHead("MQTT Setup");
        html += settingsNav("mqtt");
        html += R"rawliteral(
  <div class="container">)rawliteral";
        html += wifiWarning();
        html += R"rawliteral(
    <h2>MQTT Setup</h2>
    <form method="POST" action="/settings/mqtt">
      <label for="mqtt">MQTT Server</label>
      <input name="mqtt" id="mqtt" type="text" value=")rawliteral";
        html += String(config.mqttServer);
        html += R"rawliteral(" required>
      <label for="port">MQTT Port</label>
      <input name="port" id="port" type="number" value=")rawliteral";
        html += String(config.mqttPort);
        html += R"rawliteral(" required>
      <label for="mqttuser">MQTT User</label>
      <input name="mqttuser" id="mqttuser" type="text" value=")rawliteral";
        html += String(config.mqttUser);
        html += R"rawliteral(">
      <label for="mqttpass">MQTT Password</label>
      <input name="mqttpass" id="mqttpass" type="password" value=")rawliteral";
        html += String(config.mqttPassword);
        html += R"rawliteral(">
      <label for="mqtttopic">MQTT Topic</label>
      <input name="mqtttopic" id="mqtttopic" type="text" value=")rawliteral";
        html += String(config.mqttTopic);
        html += R"rawliteral(" required>
      <input type="submit" value="Save">
    </form>
    <a href="/" class="back-home">← Back to Home</a>
  </div>)rawliteral";
        html += getHtmlFooter();
        request->send(200, "text/html", html);
    });

    server.on("/settings/mqtt", HTTP_POST, [&](AsyncWebServerRequest *request) {
        handleSettingsUpdate(request, "MQTT", [&](Config& config) {
            strncpy(config.mqttServer, request->getParam("mqtt", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            config.mqttPort = request->getParam("port", true)->value().toInt();
            strncpy(config.mqttUser, request->getParam("mqttuser", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            strncpy(config.mqttPassword, request->getParam("mqttpass", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            strncpy(config.mqttTopic, request->getParam("mqtttopic", true)->value().c_str(), MAX_STRING_LENGTH - 1);
        }, false);
    });

    // --- Tank Settings Page ---
    server.on("/settings/tank", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);

        String html = getHtmlHead("Tank Setup");
        html += settingsNav("tank");
        html += R"rawliteral(
  <div class="container">
    <h2>Tank Setup</h2>
    <form method="POST" action="/settings/tank">
      <label for="tankDepth">Tank Depth</label>
      <div class="unit-row">
        <input name="tankDepth" id="tankDepth" type="number" step="0.1" value=")rawliteral" + String(config.tankDepth, 1) + R"rawliteral(" required>
        <select name="tankDepthUnit" id="tankDepthUnit">
          <option value="cm" )rawliteral" + (strcmp(config.outputUnit, "cm") == 0 ? "selected" : "") + R"rawliteral(>cm</option>
          <option value="in" )rawliteral" + (strcmp(config.outputUnit, "in") == 0 ? "selected" : "") + R"rawliteral(>in</option>
        </select>
      </div>
      <label for="outputUnit">Output Display Unit</label>
      <select name="outputUnit" id="outputUnit">
        <option value="cm" )rawliteral" + (strcmp(config.outputUnit, "cm") == 0 ? "selected" : "") + R"rawliteral(>Centimeters (cm)</option>
        <option value="in" )rawliteral" + (strcmp(config.outputUnit, "in") == 0 ? "selected" : "") + R"rawliteral(>Inches (in)</option>
        <option value="percent" )rawliteral" + (strcmp(config.outputUnit, "percent") == 0 ? "selected" : "") + R"rawliteral(>Percent (%)</option>
      </select>
      <input type="submit" value="Save">
    </form>
    <a href="/" class="back-home">← Back to Home</a>
  </div>
)rawliteral";
        html += getHtmlFooter();
        request->send(200, "text/html", html);
    });

    server.on("/settings/tank", HTTP_POST, [&](AsyncWebServerRequest *request)
               {
        handleSettingsUpdate(request, "Tank", [&](Config& config) {
            float depth = request->getParam("tankDepth", true)->value().toFloat();
            String depthUnit = request->getParam("tankDepthUnit", true)->value();
            if (depthUnit == "in") {
                depth *= 2.54f;
                Logger::info("Converting tank depth from inches to cm: " + String(depth));
            }
            config.tankDepth = depth;
            strncpy(config.outputUnit, request->getParam("outputUnit", true)->value().c_str(), MAX_STRING_LENGTH - 1);
        }, false);
    });

    // Add this route in setupRoutes:
    server.on("/connected", HTTP_GET, [&](AsyncWebServerRequest *request)
               {
        String ip = WiFi.localIP().toString();
        String html = getHtmlHead("Connected");
        html += R"rawliteral(
  <div class="container">
    <h2>Device Connected!</h2>
    <div class="ip">Local IP Address:<br><b>)rawliteral" + ip + R"rawliteral(</b></div>
    <div>You can now access the device at this address on your network.</div>
  </div>
)rawliteral";
        html += getHtmlFooter();
        request->send(200, "text/html", html); });

    server.on("/settings/sensor", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String html = getHtmlHead("Sensor Calibration");
        html += settingsNav("sensor");
        html += R"rawliteral(
<div class="container">
<h2>Sensor Calibration</h2>
<form method="POST" action="/settings/sensor">
  <label for="offset">Sensor Offset (cm)</label>
  <input name="offset" id="offset" type="number" step="0.1" value=")rawliteral" + String(config.sensorOffset, 1) + R"rawliteral(" required>
  <label for="full">Full Distance (cm)</label>
  <input name="full" id="full" type="number" step="0.1" value=")rawliteral" + String(config.sensorFull, 1) + R"rawliteral(" required>
  <input type="submit" value="Save">
</form>
<a href="/" class="back-home">← Back to Home</a>
</div>
)rawliteral";
        html += getHtmlFooter();
        request->send(200, "text/html", html);
    });

    server.on("/settings/sensor", HTTP_POST, [&](AsyncWebServerRequest *request)
               {
        handleSettingsUpdate(request, "Sensor", [&](Config& config) {
            config.sensorOffset = request->getParam("offset", true)->value().toFloat();
            config.sensorFull = request->getParam("full", true)->value().toFloat();
        }, false);
    });

    // --- Display Settings Page ---
    server.on("/settings/display", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        
        String html = getHtmlHead("Display Settings");
        html += settingsNav("display");
        html += R"rawliteral(
  <div class="container">
    <h2>Display Settings</h2>
    <form method="POST" action="/settings/display">
      <label for="brightness">Brightness (0-15)</label>
      <input name="brightness" id="brightness" type="number" min="0" max="15" value=")rawliteral";
        html += String(config.displayBrightness);
        html += R"rawliteral(" required>
      <label for="mode">Display Mode</label>
      <select name="mode" id="mode">
        <option value="level" )rawliteral";
        html += (strcmp(config.displayMode, "level") == 0 ? "selected" : "");
        html += R"rawliteral(>Water Level</option>
        <option value="percent" )rawliteral";
        html += (strcmp(config.displayMode, "percent") == 0 ? "selected" : "");
        html += R"rawliteral(>Percentage</option>
        <option value="raw" )rawliteral";
        html += (strcmp(config.displayMode, "raw") == 0 ? "selected" : "");
        html += R"rawliteral(>Raw Distance</option>
      </select>
      <input type="submit" value="Save">
    </form>
    <a href="/" class="back-home">← Back to Home</a>
  </div>
)rawliteral";
        html += getHtmlFooter();
        request->send(200, "text/html", html);
    });

    server.on("/settings/display", HTTP_POST, [&](AsyncWebServerRequest *request) {
        handleSettingsUpdate(request, "Display", [&](Config& config) {
            config.displayBrightness = request->getParam("brightness", true)->value().toInt();
            strncpy(config.displayMode, request->getParam("mode", true)->value().c_str(), MAX_STRING_LENGTH - 1);
        }, false);
    });

    // --- Network Settings Page ---
    server.on("/settings/network", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String html = getHtmlHead("Network Settings");
        html += settingsNav("network");
        html += R"rawliteral(
<div class="container">
<h2>Network Settings</h2>
<form method="POST" action="/settings/network">
  <label for="staticip">Static IP</label>
  <input name="staticip" id="staticip" type="text" value=")rawliteral";
    html += String(config.staticIp);
    html += R"rawliteral(" placeholder="192.168.1.100">
  <label for="gateway">Gateway</label>
  <input name="gateway" id="gateway" type="text" value=")rawliteral";
    html += String(config.gateway);
    html += R"rawliteral(" placeholder="192.168.1.1">
  <label for="subnet">Subnet Mask</label>
  <input name="subnet" id="subnet" type="text" value=")rawliteral";
    html += String(config.subnet);
    html += R"rawliteral(" placeholder="255.255.255.0">
  <label for="hostname">Hostname</label>
  <input name="hostname" id="hostname" type="text" value=")rawliteral";
    html += String(config.hostname);
    html += R"rawliteral(" placeholder="waterlevel">
  <input type="submit" value="Save">
</form>
<a href="/" class="back-home">← Back to Home</a>
</div>
)rawliteral";
        html += getHtmlFooter();
        request->send(200, "text/html", html);
    });

    server.on("/settings/network", HTTP_POST, [&](AsyncWebServerRequest *request){
        handleSettingsUpdate(request, "Network", [&](Config& config) {
            strncpy(config.staticIp, request->getParam("staticip", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            strncpy(config.gateway, request->getParam("gateway", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            strncpy(config.subnet, request->getParam("subnet", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            strncpy(config.hostname, request->getParam("hostname", true)->value().c_str(), MAX_STRING_LENGTH - 1);
        }, true);
    });

    server.on("/settings/alerts", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        String html = getHtmlHead("Alert Settings");
        html += settingsNav("alerts");
        html += R"rawliteral(
<div class="container">
<h2>Alert Settings</h2>
<form method="POST" action="/settings/alerts">
  <label for="low">Low Level Alert (%)</label>
  <input name="low" id="low" type="number" min="0" max="100" value=")" + String(config.alertLow) + R"(">
  <label for="high">High Level Alert (%)</label>
  <input name="high" id="high" type="number" min="0" max="100" value=")" + String(config.alertHigh) + R"(">
  <label for="alertmethod">Alert Method</label>
  <select name="alertmethod" id="alertmethod">
    <option value="mqtt" )" + (strcmp(config.alertMethod, "mqtt") == 0 ? "selected" : "") + R"(>MQTT</option>
    <option value="buzzer" )" + (strcmp(config.alertMethod, "buzzer") == 0 ? "selected" : "") + R"(>Buzzer</option>
    <option value="led" )" + (strcmp(config.alertMethod, "led") == 0 ? "selected" : "") + R"(>LED</option>
  </select>
  <input type="submit" value="Save">
</form>
<a href="/" class="back-home">← Back to Home</a>
</div>
)rawliteral";
        html += getHtmlFooter();
        request->send(200, "text/html", html);
    });

    server.on("/settings/alerts", HTTP_POST, [&](AsyncWebServerRequest *request){
        handleSettingsUpdate(request, "Alert", [&](Config& config) {
            config.alertLow = request->getParam("low", true)->value().toInt();
            config.alertHigh = request->getParam("high", true)->value().toInt();
            strncpy(config.alertMethod, request->getParam("alertmethod", true)->value().c_str(), MAX_STRING_LENGTH - 1);
        }, false);
    });

    // --- Device Settings Page ---
    server.on("/settings/device", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        
        String html = getHtmlHead("Device Settings");
        html += settingsNav("device");
        html += R"rawliteral(
  <div class="container">
    <h2>Device Settings</h2>
    <form method="POST" action="/settings/device">
      <label for="devicename">Device Name</label>
      <input name="devicename" id="devicename" type="text" value=")rawliteral";
        html += String(config.deviceName);
        html += R"rawliteral(" placeholder="waterlevel-01">
      <label for="ota">Enable OTA Update</label>
      <select name="ota" id="ota">)rawliteral";
        html += String("<option value=\"on\"") + (strcmp(config.otaEnabled, "on") == 0 ? " selected" : "") + ">On</option>";
        html += String("<option value=\"off\"") + (strcmp(config.otaEnabled, "off") == 0 ? " selected" : "") + ">Off</option>";
        html += R"rawliteral(
      </select>
      <input type="submit" value="Save">
    </form>
    <form method="POST" action="/settings/device/reset" onsubmit="return confirm('Are you sure you want to perform a factory reset? This will erase all settings and cannot be undone.');">
      <input type="submit" value="Factory Reset" style="background:#f44336;color:#fff;">
    </form>
    <a href="/" class="back-home">← Back to Home</a>
  </div>
)rawliteral";
        html += getHtmlFooter();
        request->send(200, "text/html", html);
    });

    server.on("/settings/device", HTTP_POST, [&](AsyncWebServerRequest *request) {
        handleSettingsUpdate(request, "Device", [&](Config& config) {
            strncpy(config.deviceName, request->getParam("devicename", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            strncpy(config.otaEnabled, request->getParam("ota", true)->value().c_str(), MAX_STRING_LENGTH - 1);
        }, false);
    });

    server.on("/settings/device/reset", HTTP_POST, [&](AsyncWebServerRequest *request) {
        Logger::info("Factory reset requested");
        Config config;
        configManager.load(config);
        
        // Reset all values to defaults
        strncpy(config.wifiSSID, "", MAX_STRING_LENGTH - 1);
        strncpy(config.wifiPassword, "", MAX_STRING_LENGTH - 1);
        strncpy(config.mqttServer, "", MAX_STRING_LENGTH - 1);
        config.mqttPort = 1883;
        strncpy(config.mqttUser, "", MAX_STRING_LENGTH - 1);
        strncpy(config.mqttPassword, "", MAX_STRING_LENGTH - 1);
        strncpy(config.mqttTopic, "home/waterlevel", MAX_STRING_LENGTH - 1);
        config.tankDepth = 100.0f;
        strncpy(config.outputUnit, "cm", MAX_STRING_LENGTH - 1);
        config.sensorOffset = 0.0f;
        config.sensorFull = 100.0f;
        config.displayBrightness = 8;
        strncpy(config.displayMode, "level", MAX_STRING_LENGTH - 1);
        strncpy(config.staticIp, "", MAX_STRING_LENGTH - 1);
        strncpy(config.gateway, "", MAX_STRING_LENGTH - 1);
        strncpy(config.subnet, "", MAX_STRING_LENGTH - 1);
        strncpy(config.hostname, "", MAX_STRING_LENGTH - 1);
        config.alertLow = 20;
        config.alertHigh = 80;
        strncpy(config.alertMethod, "mqtt", MAX_STRING_LENGTH - 1);
        strncpy(config.deviceName, "", MAX_STRING_LENGTH - 1);
        strncpy(config.otaEnabled, "off", MAX_STRING_LENGTH - 1);
        
        if (configManager.save(config)) {
            request->send(200, "text/html", getSuccessPage("Factory Reset", "Settings have been reset to defaults. The device will now restart."));
            delay(1000);
            ESP.restart();
        } else {
            request->send(500, "text/html", getErrorPage("Factory Reset Failed", "There was an error during the reset process. Please try again."));
        }
    });

    // --- WiFi Setup Page ---
    server.on("/settings/wifi", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Config config;
        configManager.load(config);
        
        String html = getHtmlHead("WiFi Setup");
        html += settingsNav("wifi");
        html += R"rawliteral(
  <div class="container">)rawliteral";
        html += wifiWarning();
        html += R"rawliteral(
    <h2>WiFi Setup</h2>
    <form method="POST" action="/settings/wifi">
      <label for="ssid">WiFi SSID</label>
      <input name="ssid" id="ssid" type="text" value=")rawliteral";
        html += String(config.wifiSSID);
        html += R"rawliteral(" required>
      <label for="wifipass">WiFi Password</label>
      <input name="wifipass" id="wifipass" type="password" value=")rawliteral";
        html += String(config.wifiPassword);
        html += R"rawliteral(">
      <input type="submit" value="Save">
    </form>
    <a href="/" class="back-home">← Back to Home</a>
  </div>)rawliteral";
        html += getHtmlFooter();
        request->send(200, "text/html", html);
    });

    server.on("/settings/wifi", HTTP_POST, [&](AsyncWebServerRequest *request) {
        handleSettingsUpdate(request, "WiFi", [&](Config& config) {
            strncpy(config.wifiSSID, request->getParam("ssid", true)->value().c_str(), MAX_STRING_LENGTH - 1);
            strncpy(config.wifiPassword, request->getParam("wifipass", true)->value().c_str(), MAX_STRING_LENGTH - 1);
        }, true);
    });

    // WiFi scan endpoint
    server.on("/scan/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
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
    server.on("/logs", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String html = getHtmlHead("Device Logs");
        html += settingsNav("logs");
        html += "<div class='container'>";
        html += "<h1>Device Logs</h1>";
        html += "<div class='logs-controls'>";
        html += "<button onclick='toggleAutoScroll()' id='autoScrollBtn'>Auto-scroll: ON</button>";
        html += "<button onclick='clearLogs()'>Clear Logs</button>";
        html += "<a href='/logs/file' class='download-logs-btn' download>Download Logs</a>";
        html += "<button onclick='viewFullLogFile()' class='view-logs-btn'>View Full Log File</button>";
        html += "</div>";
        html += "<div id='logs' class='logs-container'></div>";
        html += "<pre id='full-log-file' class='full-log-file' style='display:none;'></pre>";
        html += "</div>";
        html += getHtmlFooter();
        request->send(200, "text/html", html);
    });

    // Serve the log file for download or viewing
    server.on("/logs/file", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/logs.txt", "text/plain");
    });

    // Add clear logs endpoint
    server.on("/logs/clear", HTTP_POST, [&](AsyncWebServerRequest *request) {
        Logger::clearLogs();
        request->send(200);
    });

    // --- Help Page ---
    server.on("/help", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String html = getHtmlHead("Connection Help");
        html += settingsNav("help");
        html += R"rawliteral(
<div class="container">
<h2>Connection Guide</h2>

<div class="help-section">
    <h3>Connection Diagram</h3>
    <div class="diagram-container">
        <object type="image/svg+xml" data="/diagram.svg" style="width:100%;max-width:500px;display:block;margin:0 auto;"></object>
    </div>
</div>

<div class="help-section">
    <h3>Hardware Components</h3>
    <ul>
        <li>ESP32 Development Board</li>
        <li>JSN-SR04T Ultrasonic Sensor</li>
        <li>MAX7219 LED Matrix Display</li>
        <li>Buzzer (optional, for alerts)</li>
        <li>LED (optional, for alerts)</li>
    </ul>
</div>

<div class="help-section">
    <h3>Pin Connections</h3>
    <div class="pin-table">
        <table>
            <tr>
                <th>Component</th>
                <th>Pin</th>
                <th>ESP32 Pin</th>
                <th>Color Code</th>
            </tr>
            <tr>
                <td>JSN-SR04T Trigger</td>
                <td>TRIG</td>
                <td>GPIO 5</td>
                <td><span class="color-dot" style="background: #f44336"></span> Red</td>
            </tr>
            <tr>
                <td>JSN-SR04T Echo</td>
                <td>ECHO</td>
                <td>GPIO 18</td>
                <td><span class="color-dot" style="background: #4caf50"></span> Green</td>
            </tr>
            <tr>
                <td>MAX7219 DIN</td>
                <td>DIN</td>
                <td>GPIO 23</td>
                <td><span class="color-dot" style="background: #ff9800"></span> Orange</td>
            </tr>
            <tr>
                <td>MAX7219 CLK</td>
                <td>CLK</td>
                <td>GPIO 19</td>
                <td><span class="color-dot" style="background: #9c27b0"></span> Purple</td>
            </tr>
            <tr>
                <td>MAX7219 CS</td>
                <td>CS</td>
                <td>GPIO 21</td>
                <td><span class="color-dot" style="background: #e91e63"></span> Pink</td>
            </tr>
            <tr>
                <td>Buzzer</td>
                <td>+</td>
                <td>GPIO 22</td>
                <td><span class="color-dot" style="background: #009688"></span> Teal</td>
            </tr>
            <tr>
                <td>Alert LED</td>
                <td>+</td>
                <td>GPIO 2</td>
                <td><span class="color-dot" style="background: #795548"></span> Brown</td>
            </tr>
        </table>
    </div>
</div>

<div class="help-section">
    <h3>Power Connections</h3>
    <ul>
        <li>JSN-SR04T VCC → 5V</li>
        <li>JSN-SR04T GND → GND</li>
        <li>MAX7219 VCC → 5V</li>
        <li>MAX7219 GND → GND</li>
        <li>Buzzer GND → GND</li>
        <li>Alert LED GND → GND (with 220Ω resistor)</li>
    </ul>
</div>

<div class="help-section">
    <h3>Installation Tips</h3>
    <ul>
        <li>Mount the ultrasonic sensor at the top of the tank, facing down</li>
        <li>Ensure the sensor is perpendicular to the water surface</li>
        <li>Keep the sensor away from tank walls to avoid false readings</li>
        <li>Use shielded cables for long wire runs to reduce interference</li>
        <li>Mount the display in a visible location</li>
    </ul>
</div>

<a href="/" class="back-home">← Back to Home</a>
</div>
)rawliteral";
        html += getHtmlFooter();
        request->send(200, "text/html", html);
    });
}

void CustomWebServer::handleClient()
{
    // Not needed for AsyncWebServer, but present for interface compatibility
}

String getSuccessPage(const String& title, const String& message) {
    String html = getHtmlHead(title);
    html += R"rawliteral(
  <div class="container">
    <div class="success-message">
      <h2>)rawliteral";
    html += title;
    html += R"rawliteral(</h2>
      <p>)rawliteral";
    html += message;
    html += R"rawliteral(</p>
      <a href="/" class="back-home">← Back to Home</a>
    </div>
  </div>
)rawliteral";
    html += getHtmlFooter();
    return html;
}

String getErrorPage(const String& title, const String& message) {
    String html = getHtmlHead(title);
    html += R"rawliteral(
  <div class="container">
    <div class="error-message">
      <h2>)rawliteral";
    html += title;
    html += R"rawliteral(</h2>
      <p>)rawliteral";
    html += message;
    html += R"rawliteral(</p>
      <a href="/" class="back-home">← Back to Home</a>
    </div>
  </div>
)rawliteral";
    html += getHtmlFooter();
    return html;
}

