#pragma once

#include <ESPAsyncWebServer.h>
#include "ConfigManager.h"
#include "WaterLevelSensor.h"
#include "Config.h"

// Helper function declarations
String getHtmlHead(const String& title);
String getHtmlFooter();
String settingsNav(const String& active);
String wifiWarning();
String getSuccessPage(const String& title, const String& message);
String getErrorPage(const String& title, const String& message);

class CustomWebServer {
public:
    CustomWebServer();
    void begin(ConfigManager& configManager, WaterLevelSensor& sensor);
    void setupRoutes(ConfigManager& configManager, WaterLevelSensor& sensor);
    void handleClient(); // Not needed for AsyncWebServer, but kept for interface consistency
    void log(const String& message); // Add logging method

private:
    AsyncWebServer server;
    AsyncEventSource* _eventSource = nullptr; // Add event source for logs
};
