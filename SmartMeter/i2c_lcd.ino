#include <Wire.h> 
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x38,20,4);

SoftwareSerial GSMboard(10, 11);

byte endMsg = 0x1A;

unsigned long previousMillis = 0; 
unsigned long lastDisplayTime = 6000; 
int noSamples = 1000;

// sensor sensitivity
float vMultiplier = (220.0/12.0)*(110.0/10.0)*(5.0/1023.0);
float iMultiplier = (1000.0/1.0)*(1/33.0)*(5.0/1023.0);
void setup() {
    // LCD initialization
    lcd.init();
    lcd.backlight();

    lcd.setCursor(0,0);
    lcd.print("  SMART METER TEST  ");

    Serial.begin(9600);
    GSMboard.begin(9600);

}


void loop() {
    // float v_avg = 0.0;
    // float i_avg = 0.0;
    // int v; int i;

    // for(int j=1; j<=noSamples; j++) {
    //     v = analogRead(A0)-512;
    //     i = analogRead(A1)-512;
    //     v_avg += (vMultiplier*v)*(vMultiplier*v);
    //     i_avg += (iMultiplier*i)*(iMultiplier*i);
    //     delayMicroseconds(20);
    // }
    // v_avg /=noSamples;
    // i_avg /=noSamples;
    // float vRMS = sqrt(v_avg);
    // float iRMS = sqrt(i_avg);

    // unsigned long currentMillis = millis(); 
    // if(currentMillis-lastDisplayTime>2000){
    //     // serial display
    //     Serial.println("V: " + String(vRMS) + " I: " + String(iRMS));
    //     lastDisplayTime = currentMillis;
    // }
    String inComing = "";
    while(GSMboard.available()) {
        byte ch = GSMboard.read();
        inComing += ch;

        if(inComing.indexOf('\n') !=-1) {
            // inComing="";
            break;
        }
    }
    if(inComing!="") {
        Serial.println(inComing);
        inComing = "";
    }
}

boolean sendATcommmand(String command, String response) {

    String incoming;
    GSMboard.println(command);
    delay(1000);

    while(GSMboard.available()) {

    byte in=GSMboard.read();
    incoming +=(char)in;

    if(incoming.indexOf(response) !=-1) {
      
      incoming="";
      return true;
      
    }

    }

    incoming="";
    return false;

}