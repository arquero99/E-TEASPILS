#include "Arduino.h"
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel strip(8, 13, NEO_GRB + NEO_KHZ800);

void rainbow(int wait);

void setup(){
  Serial.begin(9600);
  strip.begin();
  strip.show();
  strip.setBrightness(50);
}

void loop() {
  rainbow(10);
}

void rainbow(int wait) {
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { 
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    delay(wait);
  }
}