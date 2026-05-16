#include <Wire.h>
#include "RTClib.h"
#include<SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

RTC_DS1307 rtc; 
LiquidCrystal_I2C lcd(0x38,20,4);
SoftwareSerial meterSerial(10,11);

// pins sensor
const byte tamperSensor1Pin = 2;
const byte tamperSensor2Pin = 3;
// Tamper Indicators
const byte tamperIndicator1Pin = 4;
const byte tamperIndicator2Pin = 5;
const byte tamperIndicator3Pin = 6;
const byte balanceIndicatorRedPin = 7;
const byte balanceIndicatorBluePin = 8;
const byte balanceIndicatorGreenPin = 9;

// variables
int tamper1State=0, tamper2State=0;
float vRMS, ipRMS, inRMS, kW, kVA, pf, kWh;
float purchasedEnergy = 1.0;
int year, month, day, hour, minute, second;
unsigned long previousMillis = 0; 
unsigned long lastDisplayTime = 6000; 
String timestamp, cvs;
// tamper flages
bool t1Flag = false, t2Flag = false, t3Flag = false;


void setup(){
	//======================================
	//===== Serial Communication Setup =====
	//======================================
	Serial.begin(9600);
	meterSerial.begin(9600);
	//======================================
	//========== LCD Display Setup =========
	//======================================
	lcd.init();
	lcd.backlight();
	lcd.setCursor(0,0);
	lcd.print("  SMART METER TEST  ");
	//======================================
	//=============== RTC Setup ============
	//======================================
	if (!rtc.begin()) {
	    // Serial.println("Couldn't find RTC");
	    while (1);
	  }
	  if (!rtc.isrunning()) {
	    // Serial.println("RTC is NOT running, setting time!");
	    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 
	  }

	// INPUT Pins
	pinMode(tamperSensor1Pin, INPUT);
	pinMode(tamperSensor2Pin, INPUT);
	// OUTPUT Pins
	pinMode(tamperIndicator1Pin, OUTPUT);
	pinMode(tamperIndicator2Pin, OUTPUT);
	pinMode(tamperIndicator3Pin, OUTPUT);
	pinMode(balanceIndicatorRedPin, OUTPUT);
	pinMode(balanceIndicatorBluePin, OUTPUT);
	pinMode(balanceIndicatorGreenPin, OUTPUT);
}

void loop(){
	// requesting data from Meter IC
	meterSerial.println("send");
	delay(500);

	char dataBuffer[100];
	if (meterSerial.available()){
		// int len = meterSerial.readBytesUntil('\n', dataBuffer, 100);
		bool receiving = false; int index;
		while(meterSerial.available()){
			char ch = meterSerial.read();
			if(ch == '<') {
				receiving = true;
				index = 0;
			} 
			else if(ch == '>') {
				receiving = false;
				dataBuffer[index] = '\0';
				break;
			} 
			else if(receiving) {
				dataBuffer[index++] = ch;
			}
		}
		// parsing the data from Meter IC
		parseData(dataBuffer);
		// Serial.println(dataBuffer));

        int kwhDigites = 5;
        float temp = kWh/10.0;
        while(temp>1.0) {
            kwhDigites -= 1;
            temp/=10.0;
        }
        // displaying parameters
        unsigned long currentMillis = millis();
	    if(currentMillis-lastDisplayTime>3000) {
	    	// Reading the RTC
	    	readRTC();
	    	// formatting data to CSV
	    	formatDataCVS();
	        // serial display
	        Serial.println("Sending Data To DB");
	        Serial.println(cvs);
	        //serialDisplay(kwhDigites);
	        // LCD display
	        lcdDisplay(kwhDigites);
	        lastDisplayTime = currentMillis;
	    }
	}

	// reading tamper sensors
	tamper1State = digitalRead(tamperSensor1Pin);
	tamper2State = digitalRead(tamperSensor2Pin);

	//===========================================
    //-------- setting tamper indicators --------
    //===========================================
    // Main Cover Sensor
	digitalWrite(tamperIndicator1Pin, tamper1State);
	if(tamper1State){t1Flag=true;}else{t1Flag=false;}
	// Terminal Cover sensor 
	digitalWrite(tamperIndicator2Pin, tamper2State);
	if(tamper2State){t2Flag=true;}else{t2Flag=false;}
	// Neutral Tamper
	if(abs(ipRMS-inRMS)>0.5) {
		digitalWrite(tamperIndicator3Pin,HIGH);
		t3Flag = true;
	} else {
		digitalWrite(tamperIndicator3Pin,LOW);
		t3Flag = false;
	}

	// delay(5000);
}

// string parsing function
float parseData(char *data) {
	char *token;
	char *strings[10];
	byte index = 0;
	token = strtok(data, ",");
	while(token!=NULL){
		strings[index] = token;
		index++;
		token = strtok(NULL, ",");
	}
	//assigning the parameters
	vRMS = atof(strings[0]);
	ipRMS = atof(strings[1]);
	inRMS = atof(strings[2]);
	kW = atof(strings[3]);
	kVA = atof(strings[4]);
	pf = atof(strings[5]);
	kWh = atof(strings[6]);
}
void formatDataCVS() {
	// making a CVS for sending
	cvs = String(vRMS)+"V,"+String(ipRMS)+"A,"+String(ipRMS)+"A,"+String(inRMS)+"A,"+String(kW)+"kW,"+String(kVA)+"kVA,";
	cvs += String(pf)+"PF,"+String(kWh)+"kWh,"+String(timestamp);
}

// Serial displaying function
void serialDisplay(int count){
    String preFix = "";
    for(int i=0;i<count;i++){
        preFix += '0';
    }
    Serial.print(String(vRMS) + "V ");
    Serial.print(String(ipRMS) + "A ");
    Serial.print(String(inRMS) + "A ");
    Serial.print(String(kW) + "kW ");
    Serial.print(String(kVA) + "kVA ");
    Serial.print(String(pf) + "PF ");
    Serial.println(preFix+ String(kWh) + "kWh");
    Serial.println("\n");
}

// LCD displaying function
void lcdDisplay(int count){
    String preFix = "";
    for(int i=0;i<count;i++){
        preFix += '0';
    }
    // clearing previous values
    lcd.clear();
    //1st row of LCD    
    if(t1Flag) {lcd.setCursor(12,0); lcd.print("T1 ");}
    if(t2Flag) {lcd.setCursor(15,0); lcd.print("T2 ");}
    if(t3Flag) {lcd.setCursor(18,0);lcd.print("T3");}
    // 2nd row of LCD
    lcd.setCursor(0,1);
    lcd.print(String(vRMS,1)+"V ");
    lcd.print(String(ipRMS,1)+"A ");
    lcd.print(String(pf)+"PF");
    // 3rd row of LCD
    lcd.setCursor(0,2);
    lcd.print(String(kW)+"kW ");
    lcd.print(String(kVA)+"kVA ");
    // 4th row of LCD
    lcd.setCursor(0,3);
    lcd.print(preFix+String(kWh)+"kWh");
}

void readRTC() {
	DateTime now = rtc.now();
	year = now.year(); month = now.month(); day = now.day();
	hour = now.hour(); minute = now.minute(); second = now.second();
	
	timestamp = String(year)+":"+String(month)+":"+String(day)+":"+String(hour)+':'+ String(minute)+':'+String(second);
}