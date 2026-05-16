// include the library code: 
#include<wire.h>
#include <Keypad.h>
#include <LiquidCrystal.h> //library for LCD
#include "EmonLib.h"

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
byte rowPins[ROWS] = {25, 26, 27, 28};
//connect to the column pin-outs of the keypad
byte colPins[COLS] = {24, 23, 22}; 
//initialize an instance of class NewKeypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// LCD Setup 
LiquidCrystal lcd(35, 34, 33, 32, 31, 30);

// Energy Monitoring Setup
EnergyMonitor emon1; 
EnergyMonitor emon2; 

// input and output pins
int keypadInterruptPin = 21;
int voltageSensorPin = A0;
int phaseCurrentSensorPin = A1;
int neutralCurrentSensorPin = A2;
int relayPin = 42;
int tamperSensor1Pin = 43;
int tamperSensor2Pin = 44;
int tamperIndicator1Pin = 45;
int tamperIndicator2Pin = 46;
int tamperIndicator3Pin = 47;
int balanceIndicatorBlue = 48;
int balanceIndicatorGreen = 49;
int balanceIndicatorRed = 50;

// sensor constants
// constants for sensor sensitivity
float vSensorSensitivity = ((100.0+10.0)/10.0)*(220.0/12.0);
float iSensorSensitivity = (1000.0/1.0)*(1.0/33.0);

// character variables
char keypressed;
// string 
String inputString = "";
// flags
bool keyPressedAtInterrupt = false;

// int variables
int tamper1State = 0;
int tamper2State = 0;
int tamper3State = 0;

// Variables for Calculations
float kWh = 0.0;
float purchasedEnergy = 1.0;
unsigned long previousMillis = 0; 
unsigned long lastDisplayTime = 6000; 

void setup(){
	//===========================================
    //----------- pin mode settings -------------
    //===========================================
	// input pins
	pinMode(tamperSensor1Pin, INPUT);
	pinMode(tamperSensor2Pin, INPUT);
	pinMode(keypadInterruptPin, INPUT);
	// output pins
	pinMode(relayPin, OUTPUT);
	pinMode(tamperIndicator1Pin, OUTPUT);
	pinMode(tamperIndicator2Pin, OUTPUT);
	pinMode(tamperIndicator3Pin, OUTPUT);
	pinMode(balanceIndicatorRed, OUTPUT);
	pinMode(balanceIndicatorBlue, OUTPUT);
	pinMode(balanceIndicatorGreen, OUTPUT);

	//===========================================
    //-------- External Interrupt settings ------
    //===========================================
    // setting interrupt for the Keypad
	attachInterrupt(digitalPinToInterrupt(keypadInterruptPin), callReadToken, RISING);

	//voltage and current measurement setting
    emon1.voltage(voltageSensorPin, vSensorSensitivity, 1.4); 
    emon1.current(phaseCurrentSensorPin, iSensorSensitivity);
    emon2.current(neutralCurrentSensorPin, iSensorSensitivity);

    //===========================================
    //----------- LCD Display settings ----------
    //===========================================
    // set up the LCD's number of columns and rows
    lcd.begin(20, 4);

	//===========================================
    //----------- Serial settings ---------------
    //===========================================
    Serial.begin(9600);

    // Setting realy pin based on available kWh
    if(purchasedEnergy>0.0){
        digitalWrite(relayPin, LOW);
    } else {
        digitalWrite(relayPin, HIGH);
    }
}

void loop() {
	// Sensor Reading
	// digital pins
	tamper1State = digitalRead(tamperSensor1Pin);
	tamper2State = digitalRead(tamperSensor2Pin);

	// energy monitoring
	// Meter Parameter Calculation
    emon1.calcVI(20,2000);
    // emon2.calcVI(20,2000);
    // Voltage and Current RMS values
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
    if(currentMillis-lastDisplayTime>5000){
        // counting number of digits before decimal point for kWh
        int kwhDigites = 5;
        float temp = kWh/10.0;
        while(temp>1.0) {
            kwhDigites -= 1;
            temp/=10.0;
        }
        // serial display
        serialDisplay(vRMS, ipRMS, kW, kVA, pf, kWh, kwhDigites);
        Serial.println(String(inRMS)+" "+String(purchasedEnergy));
        // LCD display
        lcdDisplay(vRMS, ipRMS, kW, kVA, pf, kWh, kwhDigites);

        lastDisplayTime = currentMillis;
    }

    //===========================================
    //-------- setting tamper indicators --------
    //===========================================
    setTamperIndicators(ipRMS, inRMS);
    
    // Checking for keypad press
	if(keyPressedAtInterrupt){
		inputString = readKeypad();
	}
	// token validation and verification 
	if(inputString!=""){
		Serial.println(inputString);
		lcd.setCursor(0,2);
		lcd.print(inputString);
		purchasedEnergy+=0.1;
		inputString = "";
	}
	
	// Balance checking and Load Control
	loadContol();

}
// Keypad Interrupt Interrupt Service Routine (ISR)
void callReadToken(){
	keyPressedAtInterrupt = true;
}
// Setting Indicators
void setIndicators(int red, int green, int blue){
	digitalWrite(balanceIndicatorRed, red);
	digitalWrite(balanceIndicatorBlue, blue);
	digitalWrite(balanceIndicatorGreen, green);
}
// tamper statues
void setTamperIndicators(float ipRMS, float inRMS) {
    // phase and neural current difference
    if(abs(ipRMS-inRMS)>=0.5) {
        digitalWrite(tamperIndicator3Pin, HIGH);
    } else {
        digitalWrite(tamperIndicator3Pin, LOW);
    } 
    // main cover sensor
    digitalWrite(tamperIndicator1Pin, tamper1State);
    // terminal cover sensor
    digitalWrite(tamperIndicator2Pin, tamper2State);
}

// Keypad Input Reading
String readKeypad(){
	int cursorPos = 0;
	inputString = "";
    Serial.print("Enter Token: ");
	lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Enter Token: ");
    lcd.setCursor(0,1);
	while(true){
		keypressed = keypad.getKey();
		if(keypressed){
			if(keypressed=='#'){
				break;
			} else if(keypressed=='*'){
				continue;
			} else{
				inputString+=keypressed;
				Serial.print(keypressed);
				lcd.print(keypressed);
			}
		}
	}
	keyPressedAtInterrupt = false;
	Serial.println();

	return inputString;
}

// Balance checking and Load Control
void loadContol(){
    if(purchasedEnergy>kWh){
        float diff = purchasedEnergy-kWh;
        if(diff>=0.05){
            digitalWrite(relayPin, LOW);
            setIndicators(LOW, HIGH, LOW);
        } else if(diff>=0.0 && diff < 0.05){
            digitalWrite(relayPin, LOW);
            setIndicators(LOW, LOW, HIGH);
        } else{
            digitalWrite(relayPin, HIGH);
            setIndicators(HIGH, LOW, LOW);
        }
    } else{
        digitalWrite(relayPin, HIGH);
        setIndicators(HIGH, LOW, LOW);
    }
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