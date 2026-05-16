#include <Wire.h>
#include "RTClib.h"
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

// Defining serial lines
#define gsmSerial  Serial1
#define meterSerial Serial2
// defining RTC object
RTC_DS1307 rtc; 
// defining LCD object
LiquidCrystal_I2C lcd(0x38,20,4);
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

//======================================
//==== define input and output pins ====
//======================================
// interrupt pin
int keypadInterruptPin = 3;
// sensor pins
const byte tamperSensor1Pin = 42;
const byte tamperSensor2Pin = 43;
// Tamper Indicator pins
const byte tamperIndicator1Pin = 44;
const byte tamperIndicator2Pin = 45;
const byte tamperIndicator3Pin = 46;
const byte balanceIndicatorRedPin = 47;
const byte balanceIndicatorBluePin = 49;
const byte balanceIndicatorGreenPin = 48;
// control pins
const byte relayPin = 30;

//======================================
//========== define variables ==========
//======================================
// for sensor 
int tamper1State=0, tamper2State=0;
// metering IC parameters
float vRMS, ipRMS, inRMS, kW, kVA, pf, kWh;
// for RTC
int year, month, day, hour, minute, second;
// for control
float purchasedEnergy = 0.02;
unsigned long previousMillis = 0; 
unsigned long lastDisplayTime = 6000;
// for display
String timestamp, cvs;

char var='0';

// tamper flags
bool t1Flag = false, t2Flag = false, t3Flag = false;
// keypad variables
char keypressed;
String inputString = "";
bool keyPressedAtInterrupt = false;


void setup(){
	//======================================
	//===== Serial Communication Setup =====
	//======================================
	Serial.begin(9600);
	gsmSerial.begin(9600);
	meterSerial.begin(9600);
	delay(500);

	//======================================
	//============PINMODE Setup ============
	//======================================
	// INPUT Pins
	pinMode(tamperSensor1Pin, INPUT);
	pinMode(tamperSensor2Pin, INPUT);
	pinMode(keypadInterruptPin, INPUT);
	// OUTPUT Pins
	pinMode(relayPin, OUTPUT);
	pinMode(tamperIndicator1Pin, OUTPUT);
	pinMode(tamperIndicator2Pin, OUTPUT);
	pinMode(tamperIndicator3Pin, OUTPUT);
	pinMode(balanceIndicatorRedPin, OUTPUT);
	pinMode(balanceIndicatorBluePin, OUTPUT);
	pinMode(balanceIndicatorGreenPin, OUTPUT);

	//===========================================
    //-------- External Interrupt settings ------
    //===========================================
    // setting interrupt for the Keypad
	attachInterrupt(digitalPinToInterrupt(keypadInterruptPin), callReadToken, RISING);

	//======================================
	//========== LCD Display Setup =========
	//======================================
	lcd.init();
	lcd.backlight();
	lcd.setCursor(0,1);
	lcd.print("  SMART METER TEST  ");
	lcd.setCursor(0,2);
	lcd.print("      By ESS      ");
	delay(500);

	//======================================
	//=============== RTC Setup ============
	//======================================
	rtcSetup();
	//======================================
	//======== Initial Relay Setup =========
	//======================================
	// Setting relay pin based on available kWh
    if(purchasedEnergy>0.0){
        digitalWrite(relayPin, LOW);
    } else {
        digitalWrite(relayPin, HIGH);
    }
}

void loop(){
	// reading tamper sensors
	tamper1State = digitalRead(tamperSensor1Pin);
	tamper2State = digitalRead(tamperSensor2Pin);

	unsigned long currentMillis = millis();
	if(currentMillis-lastDisplayTime>3000){
		// requesting data from Meter IC
		readMeteringIC();
		// formatting revised data
		int kwhDigites = 5;
		float temp = kWh/10.0;
		while(temp>1.0) {
			kwhDigites -= 1;
			temp/=10.0;
		}

		// Reading the RTC
		readRTC();
		delay(50);
		// formatting data to CSV
		formatDataCSV();

		//======================================
		//======= displaying parameters ========
		//======================================
		// serial display
		Serial.println("For Debugging");
		serialDisplay(kwhDigites);
		delay(100);

		// sending to database
		gsmSerial.println("Sending Data To DB");
		gsmSerial.println(cvs);
		gsmSerial.println();
		delay(100);

		// LCD display
		lcdDisplay(kwhDigites);
		lastDisplayTime = currentMillis;
    }
    //======================================
	//=========== Keypad Reading ===========
	//======================================
    // Checking for keypad press
	if(keyPressedAtInterrupt){
		inputString = readKeypadString();
		delay(50);
	}
	// token validation and verification 
	if(inputString!=""){
		Serial.println(inputString);
		lcd.setCursor(0,2);
		lcd.print(inputString);
		char arrayChar[10];
		inputString.toCharArray(arrayChar, 10);
		float purchasedKWH = atof(arrayChar);
		purchasedEnergy+=(0.1*purchasedKWH);
		inputString = "";
	}

	//===========================================
    //-------- setting tamper indicators --------
    //===========================================
    setTamperIndicators();

    //======================================
	//=========== Service control ==========
	//======================================
	loadContol();
}

// keypad Interrupt Interrupt Service Routine (ISR)
void callReadToken(){
	keyPressedAtInterrupt = true;
}

// string parsing function
void parseData(char *data) {
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

// reading data from metering IC
void readMeteringIC(){
	// requesting data from Meter IC
	meterSerial.println("send");
	delay(500);
	char dataBuffer[100];
	if (meterSerial.available()) {
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
	}
}

// keypad Input Char Reading
char readKeypadChar(){
	Serial.print("Enter: ");
	while(true){
		keypressed = keypad.getKey();
		if(keypressed){
			if(keypressed=='#') {
				break;
			} else{
				var = (char)keypressed;
				Serial.print(var);
			}
		}
	}
	Serial.println();
	keyPressedAtInterrupt = false;
	return var;
}

// keypad Input String Reading
String readKeypadString(){
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
	// Serial.println();

	return inputString;
}

// format data to CSV
void formatDataCSV() {
	// making a CVS for sending
	cvs = String(timestamp)+"-> "+String(vRMS)+","+String(ipRMS)+","+String(inRMS)+","+String(kW)+","+String(kVA);
	cvs += String(pf)+","+String(kWh)+","+String(t1Flag)+","+String(t2Flag)+","+String(t3Flag);
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
    Serial.println("Available kWh:"+String(purchasedEnergy-kWh));
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

// setting tamper statues
void setTamperIndicators() {
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
}

// setting balance Indicators
void setBalanceIndicators(int red, int green, int blue){
	digitalWrite(balanceIndicatorRedPin, red);
	digitalWrite(balanceIndicatorBluePin, blue);
	digitalWrite(balanceIndicatorGreenPin, green);
}

// balance checking and Load Control
void loadContol(){
    if(purchasedEnergy>kWh){
    	float diff = purchasedEnergy-kWh;
    	if(!t1Flag&&!t2Flag&&!t3Flag){
	        if(diff>=0.05){
	            digitalWrite(relayPin, LOW);
	            setBalanceIndicators(LOW, HIGH, LOW);
	        } else if(diff>=0.01 && diff < 0.05){
	        	// warring
	            digitalWrite(relayPin, LOW);
	            setBalanceIndicators(HIGH, HIGH, LOW);
	        } else{
	        	// cutoff
	            digitalWrite(relayPin, HIGH);
	            setBalanceIndicators(HIGH, LOW, LOW);
	        }
    	} else{
    		if(t1Flag) Serial.println("Main Cover is Open");
    		if(t2Flag) Serial.println("Terminal Cover is Open");
    		if(t3Flag) Serial.println("Current Difference");
    		digitalWrite(relayPin, HIGH);
	        setBalanceIndicators(HIGH, LOW, LOW);
    	}
    } else{
        digitalWrite(relayPin, HIGH);
        setBalanceIndicators(HIGH, LOW, LOW);
    }
}
// RTC setup
void rtcSetup(){
	if (!rtc.begin()) {
		// Serial.println("Couldn't find RTC");
		while (1);
	}
	if (!rtc.isrunning()) {
		// Serial.println("RTC is NOT running, setting time!");
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 
	}
}

// reading RTC
void readRTC() {
	DateTime now = rtc.now();
	year = now.year(); month = now.month(); day = now.day();
	hour = now.hour(); minute = now.minute(); second = now.second();
	
	timestamp = String(year)+":"+String(month)+":"+String(day)+":"+String(hour)+':'+ String(minute)+':'+String(second);
}