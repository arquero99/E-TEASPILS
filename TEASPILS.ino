#include <OLED_I2C.h>//libreria necesaria para el control de la pantala OLED
#include <Wire.h>
#include "configuration.h"
#include "SparkFun_SCD30_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SCD30
#include "ThingsBoard.h"
#include <WiFi.h>

//thingsboard
// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;
#define TOKEN               "CQwX8e4u7tCveJAPvz5b"
#define THINGSBOARD_SERVER  "iot.etsisi.upm.es"

#define WIFI_AP             "Jl"
#define WIFI_PASSWORD       "lolwifigratis"
int pinInternet=13;
boolean internetActivo= false;
boolean thingsboardActivo=false;

SCD30 airSensor;

int LIGH_SENSOR=32;
int lightValue=0;

OLED  pantalla(SDA, SCL, 8);// inicializamos la pantalla OLED

int momentoActual=0;

void setup()
{ 
  Serial.begin(115200);
  pinMode(pinInternet,OUTPUT);
  
  startDisplay();

  conectIOT(); 
  
  Wire.begin();
  if (airSensor.begin() == false)
  {
    pantalla.clrScr(); // borra la pantalla
    pantalla.print(" No se ha", CENTER, 0);//imprime la frase en el centro de la zona superior
    pantalla.print("detectado", CENTER, 10);//imprime la frase en el centro de la zona superior
    pantalla.print("  SCD30  ", CENTER, 20);//imprime la frase en el centro de la zona superior
    pantalla.update();// actualiza la pantalla    
    while (1);
  }
}

void loop () {
  
  if (WiFi.status() != WL_CONNECTED || !tb.connected()) {
    digitalWrite(pinInternet,LOW);
    conectIOT();
  }else{
    digitalWrite(pinInternet,HIGH);
    internetActivo=true;
    thingsboardActivo=true;
  }
  
  int newLight=analogRead(LIGH_SENSOR);
  if (airSensor.dataAvailable() || newLight!=lightValue)
  {
    pantalla.clrScr(); // borra la pantalla
    pantalla.print("Light: ", LEFT,30);
    pantalla.printNumI(newLight,RIGHT,30);//el numero anterior ocupa 24 pixels de alto por lo que este debe empezar a partir del 25
    
    pantalla.print("CO2: ",LEFT,0);
    pantalla.printNumI(airSensor.getCO2(),RIGHT,0);
    
    pantalla.print("Temperature: ",LEFT,10);
    pantalla.printNumI(airSensor.getTemperature(),RIGHT,10);//el numero anterior ocupa 24 pixels de alto por lo que este debe empezar a partir del 25
    
    pantalla.print("Humidity: ", LEFT,20);
    pantalla.printNumI(airSensor.getHumidity(),RIGHT,20);//el numero anterior ocupa 24 pixels de alto por lo que este debe empezar a partir del 25
    
    pantalla.update();// actualiza la pantalla
  }
  Serial.println(momentoActual);
  if(thingsboardActivo && momentoActual==envio)
  {
    Serial.println("ENVIO");
    pantalla.print("Enviando datos", LEFT,40);
    pantalla.update();// actualiza la pantalla
    tb.sendTelemetryInt("co2", airSensor.getCO2());
    tb.sendTelemetryInt("temperature", airSensor.getTemperature());
    tb.sendTelemetryInt("humidity", airSensor.getHumidity());
    tb.sendTelemetryInt("light", newLight);
    delay(1000);
    momentoActual=0;
  }
  momentoActual++;
  delay(500);
}

void conectIOT()
{  
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  if(WiFi.status() != WL_CONNECTED) {
    digitalWrite(pinInternet,LOW);
    int i=0;
    while(i<5 && WiFi.status() != WL_CONNECTED)
    {
      i++;
      delay(100);
    }
  }else{
    digitalWrite(pinInternet,HIGH);
    if (!tb.connected()){
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
        thingsboardActivo=false;
      }else{
        thingsboardActivo=true;
      }
    }
  }
}

void startDisplay()
{
  pantalla.begin();//inicializa el display OLED
  pantalla.setFont(SmallFont);//seteo el tamaño de la fuente
  pantalla.print("BIENVENIDO", CENTER, 28);//imprime la frase en el centro de la zona superior
  pantalla.update();// actualiza la pantalla
  delay(1000);
  pantalla.drawBitmap(0,0,LOGO,128,64);
  pantalla.update();// actualiza la pantalla
  delay(3000);
  pantalla.clrScr(); // borra la pantalla
}
