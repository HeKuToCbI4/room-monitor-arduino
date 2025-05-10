#include <Arduino.h>
#include "Adafruit_Sensor.h"
#include "DHT.h"
#include "MQUnifiedsensor.h"

// Configuration constants
constexpr unsigned long SAMPLING_PERIOD = 30000;  // 30 seconds
constexpr int CALIBRATION_SAMPLES = 10;
constexpr unsigned long BAUD_RATE = 112500;

// Sensor pins
constexpr int DHT_PIN = 3;
constexpr int MQ5_PIN = A0;
constexpr int MQ_135_PIN = A1;
constexpr int MQ_7_PIN = A2;

// MQ Sensor configurations
constexpr float RATIO_MQ_5_CLEAN_AIR = 6.5;
constexpr float RATIO_MQ_135_CLEAN_AIR = 3.6;
constexpr float RATIO_MQ_7_CLEAN_AIR = 27.5;

// Sensor objects
MQUnifiedsensor MQ5("Arduino UNO", 5, 10, MQ5_PIN, "MQ-5");
MQUnifiedsensor MQ135("Arduino UNO", 5, 10, MQ_135_PIN, "MQ-135");
MQUnifiedsensor MQ7("Arduino UNO", 5, 10, MQ_7_PIN, "MQ-7");
DHT dht(DHT_PIN, DHT22);

bool calibrateMQSensor(MQUnifiedsensor& sensor, const float cleanAirRatio, const char* sensorName) {
    Serial.print("Calibrating ");
    Serial.print(sensorName);
    Serial.println(", please wait...");
    
    float calcR0 = 0;
    for (int i = 1; i <= CALIBRATION_SAMPLES; i++) {
        sensor.update();
        calcR0 += sensor.calibrate(cleanAirRatio);
        Serial.print(".");
    }
    calcR0 /= CALIBRATION_SAMPLES;
    sensor.setR0(calcR0);
    Serial.println(" done!");

    if (isinf(calcR0)) {
        Serial.println("Warning: Connection issue, R0 is infinite (Open circuit detected)");
        return false;
    }
    if (calcR0 == 0) {
        Serial.println("Warning: Connection issue, R0 is zero (Analog pin shorts to ground)");
        return false;
    }
    return true;
}

void setupMQ5() {
    MQ5.setRegressionMethod(1);
    MQ5.setA(1163.8);
    MQ5.setB(-3.874);
    MQ5.init();
    
    if (!calibrateMQSensor(MQ5, RATIO_MQ_5_CLEAN_AIR, "MQ-5")) {
        Serial.println("MQ-5 initialization failed!");
    }
}

void setup() {
    Serial.begin(BAUD_RATE);
    Serial.println("Started logging.");

    // Initialize sensors
    setupMQ5();
    dht.begin();
    
    // Initialize MQ135
    if (!calibrateMQSensor(MQ135, RATIO_MQ_135_CLEAN_AIR, "MQ-135")) {
        Serial.println("MQ-135 initialization failed!");
    }

    // Initialize MQ7
    MQ7.setRegressionMethod(1);
    MQ7.setA(99.042);
    MQ7.setB(-1.518);
    MQ7.init();
    
    if (!calibrateMQSensor(MQ7, RATIO_MQ_7_CLEAN_AIR, "MQ-7")) {
        Serial.println("MQ-7 initialization failed!");
    }
}

struct SensorData {
    float temperature;
    float humidity;
    float h2;
    float alcohol;
    float co2;
    float nh4;
    float co;
};

SensorData readSensors() {
    SensorData data{};
    
    // Read DHT22
    data.temperature = dht.readTemperature();
    data.humidity = dht.readHumidity();
    
    // Read MQ5
    MQ5.update();
    data.h2 = MQ5.readSensor();
    
    // Read MQ135
    MQ135.update();
    MQ135.setA(77.255); MQ135.setB(-3.18);
    data.alcohol = MQ135.readSensor();
    
    MQ135.setA(110.47); MQ135.setB(-2.862);
    data.co2 = MQ135.readSensor();
    
    MQ135.setA(102.2); MQ135.setB(-2.473);
    data.nh4 = MQ135.readSensor();
    
    // Read MQ7
    MQ7.update();
    data.co = MQ7.readSensor();
    
    return data;
}

void printSensorData(const SensorData& data) {
    Serial.print(data.h2);
    Serial.print("|");
    Serial.print(data.temperature);
    Serial.print("|");
    Serial.print(data.humidity);
    Serial.print("|");
    Serial.print(data.alcohol);
    Serial.print("|");
    Serial.print(data.co2 + 400);
    Serial.print("|");
    Serial.print(data.nh4);
    Serial.print("|");
    Serial.print(data.co);
    Serial.println();
}

void loop() {
    const SensorData data = readSensors();
    
    // Check for invalid readings
    if (isnan(data.temperature) || isnan(data.humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }
    
    printSensorData(data);
    delay(SAMPLING_PERIOD);
}