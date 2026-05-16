#include<Wire.h>
#include "EmonLib.h"

// sensor pins
const byte voltageSensorPin = A0;
const byte phaseCurrentSensorPin = A1;
const byte neutralCurrentSensorPin = A2;

// Energy Monitoring Setup
EnergyMonitor emon1; 
EnergyMonitor emon2; 

// constants for sensor sensitivity
float vSensorSensitivity = ((100.0+10.0)/10.0)*(220.0/12.0);
float iSensorSensitivity = (1000.0/1.0)*(1.0/33.0);

// Variables for Calculations
float kWh = 0.0;
unsigned long previousMillis = 0; 
unsigned long lastDisplayTime = 6000; 

void setup(){

    //voltage and current measurement setting
    emon1.voltage(voltageSensorPin, vSensorSensitivity, 1.4); 
    emon1.current(phaseCurrentSensorPin, iSensorSensitivity);
    emon2.current(neutralCurrentSensorPin, iSensorSensitivity);

    // Serial communication
    Serial.begin(9600);

}

void loop(){

    // energy monitoring
    // Meter Parameter Calculation
    emon1.calcVI(20,2000);
    float vRMS = emon1.Vrms;
    // phase current
    float ipRMS = emon1.Irms;
    // neutral current
    float inRMS = emon2.calcIrms(1480);
    // real and apparent power
    float kW = emon1.realPower/1000.0;
    float kVA = emon1.apparentPower/1000.0;
    // power factor
    float pf = emon1.powerFactor;
    // Calculate Energy
    unsigned long currentMillis = millis();
    unsigned long elapsedTime = currentMillis - previousMillis;
    if (elapsedTime >= 1000) {
        previousMillis = currentMillis;
        kWh += (kW * (elapsedTime / 1000.0)) / 3600;
    }

    // displaying parameters
    // if(currentMillis-lastDisplayTime>500){
    //     // serial display
    //     String para = formatData(vRMS, ipRMS, inRMS, kW, kVA, pf, kWh);
    //     Serial.println(para);
    //     lastDisplayTime = currentMillis;
    // }
    String inComing="";
    if(Serial.available()){
        while (Serial.available()) {
            byte in = Serial.read();
            inComing +=(char)in;
        }
        if(inComing.indexOf("ok") !=-1) {
            String para = formatData(vRMS, ipRMS, inRMS, kW, kVA, pf, kWh);
            Serial.println(para);
            inComing="";
        }        
    }
}

// Serial displaying function
String formatData(float vRMS, float ipRMS, float inRMS, float kW, float kVA, float pf, float kWh){
    return "<"+String(vRMS)+","+String(ipRMS)+","+String(inRMS)+","+String(kW)+","+String(kVA)+","+String(pf)+","+String(kWh)+">";
}