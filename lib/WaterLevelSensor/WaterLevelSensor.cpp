#include "WaterLevelSensor.h"
#include <Arduino.h>

#define TRIGGER_PIN 5  // GPIO5
#define ECHO_PIN 18    // GPIO18
#define MEASUREMENT_INTERVAL 1000  // 1 second

WaterLevelSensor::WaterLevelSensor() : lastDistance(0), lastMeasurement(0) {}

void WaterLevelSensor::begin() {
    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIGGER_PIN, LOW);
}

void WaterLevelSensor::loop() {
    unsigned long now = millis();
    if (now - lastMeasurement >= MEASUREMENT_INTERVAL) {
        measure();
        lastMeasurement = now;
    }
}

void WaterLevelSensor::measure() {
    // Send trigger pulse
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);

    // Measure echo pulse duration
    long duration = pulseIn(ECHO_PIN, HIGH);
    
    // Convert to distance in cm
    lastDistance = duration * 0.034 / 2;
}

float WaterLevelSensor::getRawDistance() const {
    return lastDistance;
}

float WaterLevelSensor::getWaterLevelPercent() const {
    // Assuming tank height is 100cm and sensor is mounted at the top
    float tankHeight = 100.0;
    float waterLevel = tankHeight - lastDistance;
    return constrain(waterLevel / tankHeight * 100, 0, 100);
}
