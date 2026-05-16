#include <SoftwareSerial.h>

SoftwareSerial gsmBoard(10, 11);

byte endMsg = 0x1A;

void setup() {

	Serial.begin(9600);
	gsmBoard.begin(9600);
    
    while(!sendATcommmand("AT", "OK"));

    while(!sendATcommmand("AT+CIPSHUT", "OK"));

    while(!sendATcommmand("AT+CGATT=1", "OK"));

    while(!sendATcommmand("AT+CSTT=\"etc.com\", \"\", \"\"", "OK"));

    while(!sendATcommmand("AT+CIICR", "OK"));

    while(!sendATcommmand("AT+CIFSR", "OK"));

    while(!sendATcommmand("AT+CIPSTART=\"TCP\", \"HOST\", PORT", "OK"));

    while(!sendATcommmand("AT+CIPSEND", ">"));

    for(int i=50; i<5; i++){
    	gsmBoard.print("test message!!");
        gsmBoard.write(endMsg);        
    }
    gsmBoard.print("test message!!");
    gsmBoard.write(endMsg);

    while(sendATcommmand("AT+CIPCLOSE", "OK")); 
	
}

void loop() {
	
}

boolean sendATcommmand(String command, String response) {
    String inComing;
    gsmBoard.println(command);
	delay(1000);

	while(gsmBoard.available()) {
		byte in=gsmBoard.read();
		inComing +=(char)in;
		if(inComing.indexOf(response) !=-1) {
			inComing="";
			return true; 
		}
	}
	inComing="";
	return false;
}