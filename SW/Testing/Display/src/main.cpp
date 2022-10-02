#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <OneWire.h>

Adafruit_SH1106 display(21, 22);

void initDisplay();

void setup() {
  Serial.begin(9600);
  initDisplay();
}

void loop() {
  initDisplay();
  delay(5000);
}

void initDisplay(){
  //Check Display Connection 
  Wire.beginTransmission(60);
  int error = Wire.endTransmission();
  Serial.println(error);
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(45, 24);
  display.println("Starting");
  display.setCursor(25, 32);
  display.println("TEASPILS system");
  display.display();
  delay(2000);
}