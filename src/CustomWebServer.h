#pragma once

#include <ESPAsyncWebServer.h>
#include "Config.h"

class CustomWebServer {
public:
    CustomWebServer(Config& config);
    void begin();

private:
    AsyncWebServer server;
    Config& config;
}; 