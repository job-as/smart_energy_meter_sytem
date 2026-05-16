#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

void setup() {
  Serial.begin(9600);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running, setting time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 
  }
}

void loop() {
  DateTime now = rtc.now();
  Serial.print(String(now.year())+'/'+String(now.month())+'/'+String(now.day())+" ");
  Serial.print(String(now.hour())+':'+ String(now.minute())+':'+String(now.second()));
  delay(1000);
}
