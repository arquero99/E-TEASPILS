#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#define NUM_PIXELS 8
#define PIN 13

Adafruit_NeoPixel pixels(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
   pixels.begin();
  // put your setup code here, to run once:
}

void loop() {
  pixels.clear(); // Set all pixel colors to 'off'

  // The first NeoPixel in a strand is #0, second is 1, all the way up
  // to the count of pixels minus one.
  for(int i=0; i<NUM_PIXELS; i++) { // For each pixel...

    // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    // Here we're using a moderately bright green color:
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));

    pixels.show();   // Send the updated pixel colors to the hardware.

    delay(500);
  }
  // put your main code here, to run repeatedly:
}