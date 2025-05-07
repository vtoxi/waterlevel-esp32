#pragma once
#include <WString.h>
#include "ConfigManager.h"

class MQTTClient {
public:
    MQTTClient();
    bool connect(const Config& config);
    bool publish(const String& topic, const String& payload);
    void loop();
    bool isConnected() const;
private:
    String _server;
    int _port;
    String _user;
    String _password;
    String _clientId;
};
