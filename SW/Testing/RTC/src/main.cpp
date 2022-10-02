#include <Arduino.h>
#include <SPI.h>
#include "RTClib.h"

RTC_DS3231 rtc;
void initClock();

void setup() {
  Serial.begin(9600);
  initClock();

}

void loop() {
  DateTime date = rtc.now();
  String formattedDate=String(date.year()) + '/' + String(date.month()) + '/' + String(date.day()) + ' ' + String(date.hour()) + ':' + String(date.minute()) + ':' + String(date.second());
  Serial.println(formattedDate);
}

void initClock()
{
  if (!rtc.begin())
  {
    Serial.println("RTC No inicializado");
  }
  
  // Si se ha perdido la corriente, fijar fecha y hora
  if (rtc.lostPower())
  {
    // Fijar a fecha y hora de compilacion
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Fijar a fecha y hora específica. En el ejemplo, 21 de Enero de 2016 a las 03:00:00
    // rtc.adjust(DateTime(2016, 1, 21, 3, 0, 0));
  }
}
