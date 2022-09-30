#include <Arduino.h>
#include "Adafruit_Sensor.h"
#include "DHT.h"
#include "MQUnifiedsensor.h"

#define DHT_PIN 3
#define DHT_TYPE DHT22
#define MQ5_PIN A0
#define BOARD   "Arduino UNO"
#define TYPE    "MQ-5"
#define VOLTAGE_RESOLUTION 5
#define ADC_BIT_RESOLUTION 10
#define RATIO_MQ_5_CLEAN_AIR 6.5

MQUnifiedsensor MQ5(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ5_PIN, TYPE);
DHT dht = DHT(DHT_PIN, DHT_TYPE);

void setup() {
    // write your initialization code here
    Serial.begin(112500);
    Serial.println("Started logging.");
    MQ5.setRegressionMethod(1); //_PPM =  a*ratio^b
    MQ5.setA(1163.8);
    MQ5.setB(-3.874); // Configure the equation to to calculate H2 concentration
    /*
      Exponential regression:
    Gas    | a      | b
    H2     | 1163.8 | -3.874
    LPG    | 80.897 | -2.431
    CH4    | 177.65 | -2.56
    CO     | 491204 | -5.826
    Alcohol| 97124  | -4.918
    */
    MQ5.init();
    Serial.print("Calibrating please wait.");
    float calcR0 = 0;
    for (int i = 1; i <= 10; i++) {
        MQ5.update(); // Update data, the arduino will read the voltage from the analog pin
        calcR0 += MQ5.calibrate(RATIO_MQ_5_CLEAN_AIR);
        Serial.print(".");
    }
    MQ5.setR0(calcR0 / 10);
    Serial.println("  done!.");

    if (isinf(calcR0)) {
        Serial.println(
                "Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
        while (1);
    }
    if (calcR0 == 0) {
        Serial.println(
                "Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
        while (1);
    }
    /*****************************  MQ CAlibration ********************************************/
    Serial.println("Setting up DHT");
    dht.begin();
}

void loop() {
    // write your code here
    MQ5.update(); // Update data, the arduino will read the voltage from the analog pin
    float ppmH2 = MQ5.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
    float temp = dht.readTemperature();
    float humid = dht.readHumidity();
    Serial.print(ppmH2);
    Serial.print("|");
    Serial.print(temp);
    Serial.print("|");
    Serial.print(humid);
    Serial.println();
    delay(10000); //Sampling frequency
}