#include "OTAUpdateManager.h"
#include <ArduinoOTA.h>

OTAUpdateManager::OTAUpdateManager() {}

void OTAUpdateManager::begin(const char* hostname) const {
    ArduinoOTA.setHostname(hostname);

    ArduinoOTA.onStart([]() {
        // Optionally add code to handle OTA start (e.g., stop sensors)
    });
    ArduinoOTA.onEnd([]() {
        // Optionally add code to handle OTA end
    });
    ArduinoOTA.onError([](ota_error_t error) {
        // Optionally add error handling
    });

    ArduinoOTA.begin();
}

void OTAUpdateManager::handle() const {
    ArduinoOTA.handle();
}
