#pragma once
#include <Arduino.h>

class WaterLevelSensor {
public:
    WaterLevelSensor();
    void begin();
    void loop();
    float getWaterLevelPercent() const;
    float getRawDistance() const;

private:
    float lastDistance;
    unsigned long lastMeasurement;
    void measure();
};
