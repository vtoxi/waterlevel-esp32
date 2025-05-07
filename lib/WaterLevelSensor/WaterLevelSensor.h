#pragma once

class WaterLevelSensor {
public:
    WaterLevelSensor(int triggerPin, int echoPin, float tankHeightCm);

    // Returns the measured distance in centimeters
    float readDistanceCm() const;

    // Returns the water level as a percentage (0-100)
    float getWaterLevelPercent() const;

private:
    int _triggerPin;
    int _echoPin;
    float _tankHeightCm;

    // Helper to measure distance
    float measureDistance() const;
};
