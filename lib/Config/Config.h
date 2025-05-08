#pragma once

#define CONFIG_MAGIC 0x574C  // "WL" in hex
#define MAX_STRING_LENGTH 32

struct Config {
    uint16_t magic;
    char wifiSSID[32];
    char wifiPassword[32];
    char mqttServer[32];
    int mqttPort;
    char mqttUser[32];
    char mqttPassword[32];
    char mqttTopic[32];
    float tankDepth;
    char outputUnit[32];
    float sensorOffset;
    float sensorFull;
    int displayBrightness;
    char displayMode[32];
    char staticIp[32];
    char gateway[32];
    char subnet[32];
    char hostname[32];
    int alertLow;
    int alertHigh;
    char alertMethod[32];
    char deviceName[32];
    char otaEnabled[32];
}; 