#include <Wire.h>
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
char var, keypressed;
String inputString = "";
bool keyPressedAtInterrupt = false;
float purchasedEnergy = 0.20;

void setup(){

	//======================================
	//===== Serial Communication Setup =====
	//======================================
	Serial.begin(9600);
	delay(500);

	//======================================
	//============PINMODE Setup ============
	//======================================
	// INPUT Pins
	pinMode(keypadInterruptPin, INPUT);

	// setting interrupt for the Keypad
	attachInterrupt(digitalPinToInterrupt(keypadInterruptPin), callReadToken, RISING);

	Serial.println("Test...........");
	delay(1000);

}

void loop(){

	// Checking for keypad press
	if(keyPressedAtInterrupt){
		meterSetting();
	}
	// token validation and verification 
	if(inputString!=""){
		Serial.println(inputString);
		purchasedEnergy+=0.1;
		inputString = "";
	}

}

// keypad Interrupt Interrupt Service Routine (ISR)
void callReadToken(){
	keyPressedAtInterrupt = true;
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
	inputString = "";
    Serial.print("Enter: ");
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
			}
		}
	}
	keyPressedAtInterrupt = false;
	Serial.println();

	return inputString;
}

void meterSetting(){
	Serial.println("Meter Setting Enter");
	Serial.println("  1-Mode 2-Token");
	char selector = readKeypadChar();
	// Serial.println("Selector: "+selector);
	switch (selector) {
	  case '1':
	    char mode;
	    Serial.println("Mode Type");
	    Serial.println("1-prepaid 2-postpaid");
	    mode = readKeypadChar();
	    if (mode=='1'){
	    	Serial.println("The meter is set to Prepaid Mode");
	    } else if(mode=='2'){
	    	Serial.println("The meter is set to Postpaid Mode");
	    } else{
	    	Serial.println("Wrong input");
	    }
	    break;
	  case '2':
	    String stsToken="";
	    Serial.print("Enter Token: ");
	    stsToken = readKeypadString();
	    Serial.println(stsToken);
	    break;
	  default:
	    break;
	}
}