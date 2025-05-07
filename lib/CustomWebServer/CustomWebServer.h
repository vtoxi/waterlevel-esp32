#pragma once
#include "ConfigManager.h"
#include "WaterLevelSensor.h"
#include <ESPAsyncWebServer.h>

class CustomWebServer {
public:
    CustomWebServer();
    void begin(ConfigManager& configManager, WaterLevelSensor& sensor);
    void handleClient(); // Not needed for AsyncWebServer, but kept for interface consistency
    void log(const String& message); // Add logging method

private:
    AsyncWebServer _server;
    AsyncEventSource* _eventSource = nullptr; // Add event source for logs
    void setupRoutes(ConfigManager& configManager, WaterLevelSensor& sensor);
};
