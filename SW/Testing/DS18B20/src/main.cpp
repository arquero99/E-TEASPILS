#include <Arduino.h>
//Incluir las bibliotecas OneWire y DallasTemperature

#include <OneWire.h>

#include <DallasTemperature.h>

//Selecciona el pin al que se conecta el sensor de temperatura

const int oneWireBus = 14;

//Comunicar que vamos a utilizar la interfaz oneWire

OneWire oneWire(oneWireBus);

//Indica que el sensor utilizará la interfaz OneWire

DallasTemperature sensors (&oneWire);

void setup() {

  //Ajustar la velocidad para el monitor serie

  Serial.begin(9600);

  sensors.begin();

}

void loop() {

  //Leer la temperatura

  Serial.print("Mandando comandos a los sensores ");

  sensors.requestTemperatures();

  //Lectura en grados celsius

  float temperatureC1 = sensors.getTempCByIndex(0);
  float temperatureC2 = sensors.getTempCByIndex(1);

 //Escribir los datos en el monitor de serie

   Serial.print("Temperatura sensor 1: ");

  Serial.print(temperatureC1);

  Serial.println("°C");

  Serial.print("Temperatura sensor 2: ");

  Serial.print(temperatureC2);

  Serial.println("°C");

  // Lectura de la temperatura cada 5 segundos

  delay(5000);

}