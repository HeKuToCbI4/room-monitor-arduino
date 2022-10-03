#include <Arduino.h>
#include "Adafruit_Sensor.h"
#include "DHT.h"
#include "MQUnifiedsensor.h"

#define DHT_PIN 3
#define DHT_TYPE DHT22
#define MQ5_PIN A0
#define BOARD   "Arduino UNO"
#define VOLTAGE_RESOLUTION 5
#define ADC_BIT_RESOLUTION 10
#define RATIO_MQ_5_CLEAN_AIR 6.5
#define RATIO_MQ_135_CLEAN_AIR 3.6
#define RATIO_MQ_7_CLEAN_AIR   27.5
#define MQ_135_PIN A1
#define MQ_7_PIN   A2


MQUnifiedsensor MQ5(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ5_PIN, "MQ-5");
MQUnifiedsensor MQ135(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ_135_PIN, "MQ-135");
MQUnifiedsensor MQ7(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ_7_PIN, "MQ-7");
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

    Serial.println("Setting up MQ-135");
    Serial.print("Calibrating please wait.");
    float calcR0_mq_135 = 0;
    for (int i = 1; i <= 10; i++) {
        MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
        calcR0_mq_135 += MQ135.calibrate(RATIO_MQ_135_CLEAN_AIR);
        Serial.print(".");
    }
    MQ135.setR0(calcR0_mq_135 / 10);
    Serial.println("  done!.");

    if (isinf(calcR0_mq_135)) {
        Serial.println(
                "Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
        while (1);
    }
    if (calcR0_mq_135 == 0) {
        Serial.println(
                "Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
        while (1);
    }

    Serial.println("Setting up MQ-7");
    MQ7.setRegressionMethod(1); //_PPM =  a*ratio^b
    MQ7.setA(99.042); MQ7.setB(-1.518); // Configure the equation to calculate CO concentration value
    MQ7.init();
    Serial.print("Calibrating please wait.");
    float calcR0_mq7 = 0;
    for(int i = 1; i<=10; i ++)
    {
        MQ7.update(); // Update data, the arduino will read the voltage from the analog pin
        calcR0_mq7 += MQ7.calibrate(RATIO_MQ_7_CLEAN_AIR);
        Serial.print(".");
    }
    MQ7.setR0(calcR0_mq7/10);
    Serial.println("  done!.");

    if(isinf(calcR0_mq7)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
    if(calcR0_mq7 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
}

void loop() {
    // DHT22
    float temp = dht.readTemperature();
    float humid = dht.readHumidity();
    // MQ5
    MQ5.update(); // Update data, the arduino will read the voltage from the analog pin
    float ppmH2 = MQ5.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
    // MQ 135
    MQ135.update();

    MQ135.setA(77.255); MQ135.setB(-3.18); //Configure the equation to calculate Alcohol concentration value
    float Alcohol = MQ135.readSensor(); // SSensor will read PPM concentration using the model, a and b values set previously or from the setup

    MQ135.setA(110.47); MQ135.setB(-2.862); // Configure the equation to calculate CO2 concentration value
    float CO2 = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

    MQ135.setA(102.2 ); MQ135.setB(-2.473); // Configure the equation to calculate NH4 concentration value
    float NH4 = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

    // MQ-7
    MQ7.update(); // Update data, the arduino will read the voltage from the analog pin
    float CO = MQ7.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

    Serial.print(ppmH2);
    Serial.print("|");
    Serial.print(temp);
    Serial.print("|");
    Serial.print(humid);
    Serial.print("|");
    Serial.print(Alcohol);
    Serial.print("|");
    Serial.print(CO2+400);
    Serial.print("|");
    Serial.print(NH4);
    Serial.print("|");
    Serial.print(CO);
    Serial.println();
    delay(30000); //Sampling frequency
}