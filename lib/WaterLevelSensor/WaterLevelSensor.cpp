#include "WaterLevelSensor.h"
#include <Arduino.h>

WaterLevelSensor::WaterLevelSensor(int triggerPin, int echoPin, float tankHeightCm)
    : _triggerPin(triggerPin), _echoPin(echoPin), _tankHeightCm(tankHeightCm)
{
    pinMode(_triggerPin, OUTPUT);
    pinMode(_echoPin, INPUT);
}

float WaterLevelSensor::readDistanceCm() const {
    return measureDistance();
}

float WaterLevelSensor::getWaterLevelPercent() const {
    float distance = measureDistance();
    if (distance < 0) return 0.0f; // Sensor error
    float level = (_tankHeightCm - distance) / _tankHeightCm * 100.0f;
    return level < 0 ? 0.0f : (level > 100.0f ? 100.0f : level);
}

float WaterLevelSensor::measureDistance() const {
    // Send trigger pulse
    digitalWrite(_triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_triggerPin, LOW);

    // Read echo pulse
    long duration = pulseIn(_echoPin, HIGH, 30000); // 30ms timeout (~5m max)
    if (duration == 0) return -1.0f; // Timeout/error

    // Calculate distance in cm (speed of sound = 343 m/s)
    float distance = duration * 0.0343f / 2.0f;
    return distance;
}
