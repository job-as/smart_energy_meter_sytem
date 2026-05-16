#include<wire.h>
#include<SoftwareSerial.h>

SoftwareSerial meterSerial(10,11);

void setup(){
	Serial.begin(9600);
	meterSerial.begin(9600);

}
void loop(){

	// requesting data from Meter IC
	meterSerial.println("ok");
	delay(50);

	char dataBuffer[100];
	if (meterSerial.available()){
		int len = meterSerial.readBytesUntil('\n', dataBuffer, 100);
		dataBuffer[len] = '\0'; 
		float vRMS = parseData(dataBuffer, 0);
		float ipRMS = parseData(dataBuffer, 1);
		float inRMS = parseData(dataBuffer, 2);
		float kVA = parseData(dataBuffer, 4);
		float kWh = parseData(dataBuffer, 6);
		float kW = parseData(dataBuffer, 3);
		float pf = parseData(dataBuffer, 5);

        // counting number of digits before decimal point for kWh
        int kwhDigites = 5;
        float temp = kWh/10.0;
        while(temp>1.0) {
            kwhDigites -= 1;
            temp/=10.0;
        }
        // serial display
        serialDisplay(vRMS, ipRMS, inRMS, kW, kVA, pf, kWh, kwhDigites);
        // LCD display
	}

}

float parseData(char *data, byte pos) {
	char *token;
	char *strings[10];
	byte index = 0;
	token = strtok(data, ",");
	while(token!=NULL){
		strings[index] = token;
		index++;
		token = strtok(NULL, ",");
	}
	
	return atof(strings[pos]);
}

// Serial displaying function
void serialDisplay(float vRMS, float ipRMS, float inRMS, float kW, float kVA, float pf, float kWh, int count){
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