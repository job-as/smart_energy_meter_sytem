// include the library code: 
#include<wire.h>
#include <LiquidCrystal.h> //library for LCD 
#include "EmonLib.h"   // Include Emon Library
#include <Keypad.h>

// Keypad Setups
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
        {'1','2','3'},
        {'4','5','6'},
        {'7','8','9'},
        {'*','0','#'}
    };
//connect to the row pin-outs of the keypad
byte rowPins[ROWS] = {24, 25, 26, 27};
//connect to the column pin-outs of the keypad
byte colPins[COLS] = {23, 22, 21}; 
//initialize an instance of class NewKeypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );


// LCD Setup 
LiquidCrystal lcd(35, 34, 33, 32, 31, 30); 

// Energy Monitoring Setup
EnergyMonitor emon1; 
EnergyMonitor emon2; 

// sensor pins
int keypadInterruptPin = 21;
int t1SensorPin = 18;
int t2SensorPin = 19;
int vSensorPin = A8;
int cpSensorPin = A9;
int cnSensorPin = A10;

//output pins
int t1IndicatorPin = 14;
int t2IndicatorPin = 15;
int t3IndicatorPin = 16;
int relayPin = 17;

// sensor constants
// constants for sensor sensitivity
float vSensorSensitivity = ((100.0+10.0)/10.0)*(220.0/12.0);
float cSensorSensitivity = (1000.0/1.0)*(1.0/33.0);

// variables
int t1State = 0;
int t2State = 0;
String token ="";
char keypressed = ' ';
// prechused energy
float purchasedEnergy = 0.0;

// flags
bool keyPressedAtInterrupt = false;

// Variables for Calculations
unsigned long previousMillis = 0; 
unsigned long lastDisplayTime = 6000; 
float kWh = 0.0;

void setup() {
    //===========================================
    //----------- pin mode settings -------------
    //===========================================
    //input pins
    pinMode(t1SensorPin,INPUT);
    pinMode(t2SensorPin,INPUT);
    pinMode(keypadInterruptPin, INPUT);
    //output pins
    pinMode(relayPin,OUTPUT);
    pinMode(t1IndicatorPin,OUTPUT);
    pinMode(t2IndicatorPin,OUTPUT);
    pinMode(t3IndicatorPin,OUTPUT);    

    //===========================================
    //---------- Interrupt settings -------------
    //===========================================
    // setting interrupt for the Keypad
    attachInterrupt(digitalPinToInterrupt(keypadInterruptPin), callReadToken, RISING);

    //===========================================
    //-------- Energy Monitoring settings -------
    //===========================================
    //voltage and current measurement setting
    emon1.voltage(vSensorPin, vSensorSensitivity, 0.0); 
    emon1.current(cpSensorPin, cSensorSensitivity); 
    // emon2.current(cnSensorPin, cSensorSensitivity);

    //===========================================
    //----------- LCD Display settings ----------
    //===========================================
    // set up the LCD's number of columns and rows
    lcd.begin(20, 4);
    //===========================================
    //----------- Serial settings ---------------
    //===========================================
    Serial.begin(9600);
}

void loop() {
    //===========================================
    //----------- Reading inputs ----------------
    //===========================================
    // digital pins
    t1State = digitalRead(t1SensorPin);
    t2State  = digitalRead(t2SensorPin);
    //analog pins
    ADMUX = 0x40;
    int ip = analogRead(cpSensorPin);
    int in = analogRead(cnSensorPin);

    // Meter Parameter Calculation
    emon1.calcVI(20,2000);
    // emon2.calcVI(20,2000);
    // Voltage and Current RMS values
    float vRMS = emon1.Vrms;
    // phase current
    float ipRMS = emon1.Irms;
    // neutral current
    // float inRMS = emon2.Irms;
    // real and apparent power
    float kW = emon1.realPower/1000.0;
    float kVA = emon1.apparentPower/1000.0;
    // power factor
    float pf = emon1.powerFactor;
    // Calculate Energyte%tem
    unsigned long currentMillis = millis();
    unsigned long elapsedTime = currentMillis - previousMillis;
    if (elapsedTime >= 1000) {
        previousMillis = currentMillis;
        kWh += (kW * (elapsedTime / 1000.0)) / 3600;
    }
    // counting number of digits before decimal point for kWh
    int kwhDigites = 5;
    float temp = kWh/10.0;
    while(temp>1.0){
        kwhDigites -= 1;
        temp/=10.0;
    }

    // displaying parameters
    if(currentMillis-lastDisplayTime>5000){
        // serial display
        serialDisplay(vRMS, ipRMS, kW, kVA, pf, kWh, kwhDigites);
        // LCD display
        lcdDisplay(vRMS, ipRMS, kW, kVA, pf, kWh, kwhDigites);

        lastDisplayTime = currentMillis;
    }
    
    if(keyPressedAtInterrupt){
        token = readToken();
        keyPressedAtInterrupt = false;
    }

    if(token != ""){
        Serial.println("Checking Validity of the token!!!");
        purchasedEnergy += 0.1;
        token = "";
    }
    // Service control
    if(purchasedEnergy >= kWh){    
        float diff = purchasedEnergy-kWh;    
        if(diff >= 0.05 && diff <= 0.1){
            Serial.println("Warring");
            lcd.setCursor(10,0);
            lcd.print("Warring");
            Serial.println(purchasedEnergy);        
            digitalWrite(relayPin, LOW);
        } else if(diff <= 0.05){
            Serial.println("Recharge");
            lcd.setCursor(10,0);
            lcd.print("Recharge");
            // Serial.println(purchasedEnergy);        
            digitalWrite(relayPin, HIGH);
        } else{
            digitalWrite(relayPin, LOW);
            // Serial.println(purchasedEnergy);        
        }
    } else{
        digitalWrite(relayPin, HIGH);
        Serial.println("Recharge");        
    }

    //===========================================
    //-------- setting tamper indicators --------
    //===========================================
    // main cover tamper sensor
    digitalWrite(t1IndicatorPin, t1State);
    // terminal cover tamper sensors
    digitalWrite(t2IndicatorPin, t2State);
    // Phase and Neutral tamper sensor
    if(abs(ip-in)>50){
        digitalWrite(t3IndicatorPin, HIGH);
    } else {
        digitalWrite(t3IndicatorPin, LOW);
    }
    delay(500);

}

// Keypad Interrupt Interrupt Service Routine (ISR)
void callReadToken(){
    keyPressedAtInterrupt = true;
}

// Keypad Character reading function
String readToken(){
    int cursorPos = 0;
    token = "";
    lcd.clear();
    Serial.print("Enter Token: ");
    lcd.setCursor(0,0);
    lcd.print("Enter Token: ");
    lcd.setCursor(0,1);
    while(true){
        keypressed = keypad.getKey();
        if(keypressed=='*'){
            Serial.println();
            break;
        } else if(keypressed){
            if(keypressed=='#'){
                // cursorPos-=1;
                // lcd.setCursor(cursorPos,1);
                continue;
            }
            Serial.print(keypressed);
            lcd.print(keypressed);
            token += String(keypressed);
            cursorPos+=1;
        } 
    }
    Serial.println(token);

    return token;
}
// LCD displaying function
void lcdDisplay(float vRMS, float ipRMS, float kW, float kVA, float pf, float kWh, int count){
    String preFix = "";
    for(int i=0;i<count;i++){
        preFix += '0';
    }
    // clearing previous values
    lcd.clear();
    // 2nd row of LCD
    lcd.setCursor(0,1);
    lcd.print(String(vRMS)+"V ");
    lcd.print(String(ipRMS)+"A ");
    lcd.print(String(pf)+"PF");
    // 3rd row of LCD
    lcd.setCursor(0,2);
    lcd.print(String(kW)+"kW ");
    lcd.print(String(kVA)+"kVA ");
    // 4th row of LCD
    lcd.setCursor(0,3);
    lcd.print(preFix+String(kWh)+"kWh");
}
// Serial displaying function
void serialDisplay(float vRMS, float ipRMS, float kW, float kVA, float pf, float kWh, int count){
    String preFix = "";
    for(int i=0;i<count;i++){
        preFix += '0';
    }
    Serial.print(String(vRMS) + "V ");
    Serial.print(String(ipRMS) + "A ");
    Serial.print(String(kW) + "kW ");
    Serial.print(String(kVA) + "kVA ");
    Serial.print(String(pf) + "PF ");
    Serial.println(preFix+ String(kWh) + "kWh");
    Serial.println("\n");
}