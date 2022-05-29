#include <Arduino.h>

#include <credentials.h>
#include <configuracion.h>
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SCD30
#include "ThingsBoard.h"
//#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_VEML7700.h"



//wifi data
#define WIFI_NAME  "Jl"//wifi
#define WIFI_PASSWORD  "lolwifigratis"//wifipass

int pinInternet=4;
boolean internetActivo= false;

//thingsboard
#define TOKEN   tokenDevice
#define THINGSBOARD_SERVER  thingsboardServer
// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;
boolean thingsboardActivo=false;
int pinThingsboard=16;

//telegram

#define BOT_TOKEN tokenBot
const unsigned long BOT_MTBS = 1000; // mean time between scan messages
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime; // last time messages' scan has been done

//real time clock
RTC_DS3231 rtc;

//SD
int pinSD=17;
boolean sdActiva=false;
File logData;
int hora=0;

//SCD30 Sensor
SCD30 airSensor;

//veml7700
Adafruit_VEML7700 veml = Adafruit_VEML7700();

//led
Adafruit_NeoPixel pixel(1, 4, NEO_GRB + NEO_KHZ800);

//LED_RING
Adafruit_NeoPixel pixels(NUMPIXELS, Ring_PIN, NEO_GRB + NEO_KHZ800);
int ringElement=1;

//OLED
Adafruit_SH1106 display(21, 22);

//contador para lectura de datos
int ambCont = 0;
int soilCont = 0;

int lightValue = 0;
int CO2Value = 0;
int tempValue = 0;
int humidityValue = 0;
int soilHumidityValue = 0;
int soilTemperatureValue = 0;

int presentMoment = 0;

void comprobarSD();
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
String completeDate();
void conectIOT();
void startDisplay();
void lecturaSensores();
void mostrarCarrusel();
void mostrarTodo();
void fixSensor(int sensor);
void escrituraSensores();
void comprobarBot();
void handleNewMessages(int numNewMessages);
void ring(int sensor);
void RING_LIGHT();
void RING_CO2();
void RING_TEMP();
void RING_HUM();
void RING_SOILHUM();
void RING_SOILTEM();

void setup()
{ 
  Serial.begin(115200);

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();
  
  pixel.begin(); 
  pixel.clear();

  pixel.setPixelColor(0, pixel.Color(0,0, 0));
  pixel.show();

  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  startDisplay(); //Se inicia la pantalla OLED presentando un mensaje de bienvenida y el logo
  
  delay(2000); // Pause for 2 seconds
  
  //Se comprueba si hay microSD evitando continuar hasta que no se inserte
  if (!SD.begin(5))
  {
    sdActiva=false;  
  //  digitalWrite(pinSD,HIGH);    
    display.clearDisplay();
    display.setCursor(45,24);
    display.println("Problema");//imprime la frase en el centro de la zona superior
    display.setCursor(25,32);
    display.println("en la SD");//imprime la frase en el centro de la zona superior
    display.display();
    while(!SD.begin(5));
  }
  
  sdActiva=true;
  //digitalWrite(pinSD,LOW);    
      
  writeFile(SD, "/log.txt", LogInitialMessage.c_str());
  
  //logData = SD.open("/log.txt", FILE_WRITE);

  //Se conecta a WIFI y la plataforma IoT
  conectIOT(); 

  //Se inicializa el sensor de CO2, Temperatura y humedad ambiental
  Wire.begin();
  while (airSensor.begin() == false)
  {
    display.clearDisplay();
    display.setCursor(45,24);
    display.println(" No se ha ");//imprime la frase en el centro de la zona superior
    display.setCursor(45,32);
    display.println(" detectado");//imprime la frase en el centro de la zona superior
    display.setCursor(45,40);
    display.println("   SCD30  ");//imprime la frase en el centro de la zona superior
    display.display();
  }

  while (veml.begin() == false)
  {
    display.clearDisplay();
    display.setCursor(45,24);
    display.println(" No se ha ");//imprime la frase en el centro de la zona superior
    display.setCursor(45,32);
    display.println(" detectado");//imprime la frase en el centro de la zona superior
    display.setCursor(45,40);
    display.println(" veml7700 ");//imprime la frase en el centro de la zona superior
    display.display();
  }
  veml.setGain(VEML7700_GAIN_1);
  veml.setIntegrationTime(VEML7700_IT_800MS);
  veml.setLowThreshold(10000);
  veml.setHighThreshold(20000);
  veml.interruptEnable(true);
  
  //Se inicializa el reloj 
  while (!rtc.begin()){    
    display.clearDisplay();
    display.setCursor(45,24);
    display.println(" No se ha ");//imprime la frase en el centro de la zona superior
    display.setCursor(45,32);
    display.println(" detectado");//imprime la frase en el centro de la zona superior
    display.setCursor(45,40);
    display.println("   reloj  ");//imprime la frase en el centro de la zona superior
    display.display();
  }
  
  // Si se ha perdido la corriente, fijar fecha y hora
  if (rtc.lostPower()) {
    // Fijar a fecha y hora de compilacion
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Fijar a fecha y hora específica. En el ejemplo, 21 de Enero de 2016 a las 03:00:00
    // rtc.adjust(DateTime(2016, 1, 21, 3, 0, 0));
  }
  
}

void loop () {

  comprobarSD();
  
  if (WiFi.status() != WL_CONNECTED || !tb.connected()) {    
    conectIOT();
  }else{
    internetActivo=true;
    thingsboardActivo=true;
  }

  comprobarBot();
  
  lecturaSensores();

  if(displayMode==3)
    mostrarCarrusel();
  else
    if(displayMode==1)
      mostrarTodo();
        else
          fixSensor(fixedSensor);
    
  escrituraSensores();

  //if(thingsboardActivo && presentMoment==envio)
  if(presentMoment==envio)
  {
    Serial.println("ENVIO DATOS");
    tb.sendTelemetryInt("co2", CO2Value);
    tb.sendTelemetryInt("temperature", tempValue);
    tb.sendTelemetryInt("humidity", humidityValue);
    tb.sendTelemetryInt("light", lightValue);
    tb.sendTelemetryInt("soilTemp", soilTemperatureValue);
    tb.sendTelemetryInt("soilHumidity", soilHumidityValue);
    presentMoment=0;
  }
  
  ambCont++;
  soilCont++;
  presentMoment++;

  delay(tiempoDelay);
}

void comprobarSD()
{
  SD.end();
  if (SD.begin(5))
  {
    sdActiva=true;
    //digitalWrite(pinSD,LOW);    
    //logData = SD.open("/log.txt", FILE_WRITE);
    
  }else{
    sdActiva=false;  
    pixel.setPixelColor(0, pixel.Color(0,0, 255));
    pixel.show();    display.clearDisplay();
    display.setCursor(45,24);
    display.println("Problema");//imprime la frase en el centro de la zona superior
    display.setCursor(25,32);
    display.println("en la SD");//imprime la frase en el centro de la zona superior
    display.display();
    while(!SD.begin(5));
    sdActiva=true;
    digitalWrite(pinSD,LOW);    
    //logData = SD.open("/log.txt", FILE_WRITE);
  }  
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  //Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    //Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    //Serial.println("File written");
  } else {
    //Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) {

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    return;
  }
  if (file.print(message)) {
    //Serial.println("Message appended");
  } else {
    //Serial.println("Append failed");
  }
  file.close();
}

String completeDate()
{
  DateTime date = rtc.now();
  String date2=String(date.year())+'/'+String(date.month())+'/'+String(date.day())+' '+String(date.hour())+':'+String(date.minute())+':'+String(date.second());
  return date2;
}

void conectIOT()
{  
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  if(WiFi.status() != WL_CONNECTED) {
    
    for(int i=0;i<10 && WiFi.status() != WL_CONNECTED; i++)
    {
      delay(500);
    } 

    pixel.setPixelColor(0, pixel.Color(255,0, 0));
    pixel.show();
    
    int i=0;
    while(i<5 && WiFi.status() != WL_CONNECTED)
    {
      i++;
      delay(100);
    }
  }else{

    //pixel.setPixelColor(0, pixel.Color(255,255, 0));
    //pixel.show();
    
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
    if (!tb.connected()){
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
        thingsboardActivo=false;
        pixel.setPixelColor(0, pixel.Color(0,255, 0));
        pixel.show();
      }else{
        thingsboardActivo=true;
        pixel.setPixelColor(0, pixel.Color(0,0,0));
        pixel.show();
      }
    }
  }
}

void startDisplay()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(45,24);
  display.println("Starting");
  display.setCursor(25,32); 
  display.println("TEASPIL system");
  display.display();
  delay(3000);

  // miniature bitmap display
  display.clearDisplay();
  display.drawBitmap(0, 0,  LOGO, 128, 64, 1);
  display.display();
  delay(3000);
}

void lecturaSensores()
{
  if(ambCont == ambCadence)
  {
    lightValue = veml.readLux();
    CO2Value = airSensor.getCO2();
    tempValue = airSensor.getTemperature();
    humidityValue = airSensor.getHumidity();  

    ambCont=0;
  }
  
  if(soilCont == soilCadence) 
  {
    soilHumidityValue = analogRead(SOILHUMIDITY_SENSOR_PIN);
    soilTemperatureValue = analogRead(SOILTEMPERATURE_PIN);

    soilCont=0;
  }
}

void mostrarCarrusel()
{
  ring(2);
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(50,0);
  display.print("CO2:");
  display.setCursor(0,50);
  display.print(CO2Value);
  display.display(); 
  delay(cambio);  

  ring(3);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("Temperature:");
  display.setCursor(0,50);
  display.print(tempValue);
  display.display(); 
  delay(cambio); 

  ring(4);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(5,0);
  display.print("Humidity:");
  display.setCursor(0,50);
  display.print(humidityValue);
  display.display(); 
  delay(cambio);  

  ring(1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(30,0);
  display.print("Light:");
  display.setCursor(0,50);
  display.print(lightValue);
  display.display(); 
  delay(cambio);

  ring(6);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(5,0);
  display.print("Soil Temp:");
  display.setCursor(0,50);
  display.print(soilTemperatureValue);
  display.display(); 
  delay(cambio);

  ring(5);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("Soil Humidity:");
  display.setCursor(0,50);
  display.print(soilHumidityValue);
  display.display(); 
  delay(cambio);
}

void mostrarTodo()
{
  ring(ringElement);
  String isco2="";
  String istem="";
  String ishum="";
  String islig="";
  String issoil="";
  String isloud="";

  switch(ringElement)
  {
    case 1:islig="*";
    break;
    case 2:isco2="*";
    break;
    case 3:istem="*";
    break;
    case 4:ishum="*";
    break;
    case 5:issoil="*";
    break;
    case 6:isloud="*";
    break;
  }
    
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("CO2:"+isco2);
  display.setCursor(100,0);
  display.println(CO2Value);

  display.setCursor(0,10);
  display.print("Temperature:"+istem);
  display.setCursor(100,10);
  display.print(tempValue);
  
  display.setCursor(0,20);
  display.print("Humidity:"+ishum);
  display.setCursor(100,20);
  display.print(humidityValue);
  
  display.setCursor(0,30);
  display.print("Light:"+islig);
  display.setCursor(100,30);
  display.print(lightValue);
  
  display.setCursor(0,40);
  display.print("Soil Temp:"+isloud);
  display.setCursor(100,40);
  display.print(soilTemperatureValue);
  
  display.setCursor(0,50);
  display.print("Soil Humidity:"+issoil);
  display.setCursor(100,50);
  display.print(soilHumidityValue);
  
  display.display();
}

void fixSensor(int sensor)
{
  switch(sensor){
  case 2:{
    ring(2);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print("CO2:");
    display.setCursor(0,28);
    display.print(CO2Value);
    display.display(); 
  }
  break;
  case 3:{
    ring(3);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print("Temperature:");
    display.setCursor(0,28);
    display.print(tempValue);
    display.display(); 
  }
  break;
  case 4:{    
    ring(4);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print("Humidity:");
    display.setCursor(0,28);
    display.print(humidityValue);
    display.display(); 
  }
  break;
  case 1:{
    ring(1);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print("Light:");
    display.setCursor(0,28);
    display.print(lightValue);
    display.display(); 
  }
  break;
  case 6:{
    ring(6);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print("Soil Temp:");
    display.setCursor(0,28);
    display.print(soilTemperatureValue);
    display.display(); 
  }
  break;
  case 5:{
    ring(5);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print("Soil Humidity:");
    display.setCursor(0,28);
    display.print(soilHumidityValue);
    display.display(); 
  }
  break;
  }
}

void escrituraSensores()
{
  String CO2Message = completeDate()+";"+CO2_ID+";"+CO2Value;
  String TempMessage = completeDate()+";"+Temp_ID+";"+tempValue;
  String HumidityMessage = completeDate()+";"+Humidity_ID+";"+humidityValue;  
  String LightMessage = completeDate()+";"+Light_ID+";"+lightValue;
  String SoilTemperatureMessage = completeDate()+";"+SoilTemperature_ID+";"+humidityValue;
  String soilHumidityMessage = completeDate()+";"+SoilHumidity_ID+";"+soilHumidityValue;
  
  String writeLog = "\n"+ CO2Message +"\n"+ TempMessage +"\n"+ HumidityMessage +"\n"+ LightMessage +"\n"+ SoilTemperatureMessage +"\n"+ soilHumidityMessage;
  
  appendFile(SD, "/log.txt", writeLog.c_str());
  
}

void comprobarBot()
{
  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
}

void handleNewMessages(int numNewMessages)
{

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    //String text = bot.messages[i].text;
    String s = bot.messages[i].text;
    //Serial.println(s);
    String text;
    String param1;
    String param2;
    String param3;

    String delimiter = " ";
    size_t pos = 0;
    String token;
    if((pos = s.indexOf(delimiter)) != -1 )
    {
      token = s.substring(0, pos);
      text = token; 
      //Serial.println(text);
      s.remove(0, pos + delimiter.length());
      if((pos = s.indexOf(delimiter)) != -1 )
      {
        token = s.substring(0, pos);
        param1 = token; 
        s.remove(0, pos + delimiter.length());

        if((pos = s.indexOf(delimiter)) != -1 )
        {
          token = s.substring(0, pos);
          param2 = token; 
          s.remove(0, pos + delimiter.length());
          token = s.substring(0);
          param3 = token; 
          s.remove(0, pos + delimiter.length());
        }
      }
      
    }else{
      text=s;
    }
     

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/carousel")
    {
      displayMode = 3;
      bot.sendMessage(chat_id, "Carousel mode", "");
    }

    if (text == "/all")
    {      
      displayMode = 1;
      bot.sendMessage(chat_id, "Screen showing all", "");
    }

    if (text == "/ringLight")
    {      
      ringElement = 1;
      bot.sendMessage(chat_id, "Ring show now the light", "");
    }

    if (text == "/ringCO2")
    {      
      ringElement = 2;
      bot.sendMessage(chat_id, "Ring show now the co2", "");
    }

    if (text == "/ringTemperature")
    {      
      ringElement = 3;
      bot.sendMessage(chat_id, "Ring show now the temperature", "");
    }

    if (text == "/ringHumidity")
    {      
      ringElement = 4;
      bot.sendMessage(chat_id, "Ring show now the humidity", "");
    }

    if (text == "/ringSoilHumidity")
    {      
      ringElement = 5;
      bot.sendMessage(chat_id, "Ring show now the soil humidity", "");
    }

    if (text == "/ringSoilTemperature")
    {      
      ringElement = 6;
      bot.sendMessage(chat_id, "Ring show now the soil temperature", "");
    }

    if (text == "/setLight")
    {     
      displayMode = 2; 
      fixedSensor = 1;
      bot.sendMessage(chat_id, "Display show now the light", "");
    }

    if (text == "/setCO2")
    { 
      displayMode = 2;      
      fixedSensor = 2;
      bot.sendMessage(chat_id, "Display show now the co2", "");
    }

    if (text == "/setTemperature")
    {    
      displayMode = 2;  
      fixedSensor = 3;
      bot.sendMessage(chat_id, "Display show now the temperature", "");
    }

    if (text == "/setHumidity")
    {      
      displayMode = 2;
      fixedSensor = 4;
      bot.sendMessage(chat_id, "Display show now the humidity", "");
    }

    if (text == "/setSoilHumidity")
    {      
      displayMode = 2;
      fixedSensor = 5;
      bot.sendMessage(chat_id, "Display show now the soil humidity", "");
    }

    if (text == "/setSoilTemperature")
    {      
      displayMode = 2;
      fixedSensor = 6;
      bot.sendMessage(chat_id, "Display show now the soil temperature", "");
    }

    if (text == "/getLight")
    {
      String message="Light: "+(String)lightValue;
      bot.sendMessage(chat_id, message, "");
    }
    if (text == "/getCO2")
    {
      String message="CO2: "+(String)CO2Value;
      bot.sendMessage(chat_id, message, "");
    }
    if (text == "/getTemperature")    
    {
      String message="Temperature: "+(String)tempValue;
      bot.sendMessage(chat_id, message, "");
    }
    if (text == "/getHumidity")    
    {
      String message="Humidity: "+(String)humidityValue;
      bot.sendMessage(chat_id, message, "");
    }
    if (text == "/getSoilHumidity")
    {
      String message="Soil Humidity: "+(String)soilHumidityValue;
      bot.sendMessage(chat_id, message, "");
    }
    if (text == "/getSoilTemperature")
    {
      String message="Soil Temperature: "+(String)soilTemperatureValue;
      bot.sendMessage(chat_id, message, "");
    }

    if (text == "SetColorQuartile1")
    {      
      quartile1[0] = param1.toInt();
      quartile1[1] = param2.toInt();
      quartile1[2] = param3.toInt();
      bot.sendMessage(chat_id, "Updated the quartile-1 color", "");
    }

    if (text == "SetColorQuartile2")
    {      
      quartile2[0] = param1.toInt();
      quartile2[1] = param2.toInt();
      quartile2[2] = param3.toInt();
      bot.sendMessage(chat_id, "Updated the quartile-2 color", "");
    }

    if (text == "SetColorQuartile3")
    {      
      quartile3[0] = param1.toInt();
      quartile3[1] = param2.toInt();
      quartile3[2] = param3.toInt();
      bot.sendMessage(chat_id, "Updated the quartile-3 color", "");
    }

    if (text == "SetColorQuartile4")
    {      
      quartile4[0] = param1.toInt();
      quartile4[1] = param2.toInt();
      quartile4[2] = param3.toInt();
      bot.sendMessage(chat_id, "Updated the quartile-4 color", "");
    }
    if(text == "/COLOR")
    {
      String aux=(String)quartile1[0]+" "+(String)quartile1[1]+" "+(String)quartile1[2];
      bot.sendMessage(chat_id, aux, "");
    }
    if (text == "/start")
    {
      String welcome = "Bienvenido a la configuracion del sistema TEASPILS, " + from_name + ".\n";
      welcome += "/carousel : Show one by one\n";
      welcome += "/all : Show all\n";
      welcome += "/ringLight : Show light value in the ring\n";
      welcome += "/ringCO2 : Show CO2 value in the ring\n";
      welcome += "/ringTemperature : Show temperature value in the ring\n";
      welcome += "/ringHumidity : Show humidity value in the ring\n";
      welcome += "/ringSoilHumidity : Show soil humidity value in the ring\n";
      welcome += "/ringSoilTemperature : Show soil temperature value in the ring\n";
      welcome += "/getLight : actual light value\n";
      welcome += "/getCO2 : actual CO2 value\n";
      welcome += "/getTemperature : actual temperature value\n";
      welcome += "/getHumidity : actual humidity value\n";
      welcome += "/getSoilHumidity : actual soil humidity value\n";
      welcome += "/getSoilTemperature : actual soil temperature value\n";
      welcome += "/setLight : set light value in display\n";
      welcome += "/setCO2 : set CO2 value in display\n";
      welcome += "/setTemperature : set temperature value in display\n";
      welcome += "/setHumidity : set humidity value in display\n";
      welcome += "/setSoilHumidity : set soil humidity value in display\n";
      welcome += "/setSoilTemperature : set soil temperature value in display\n";
      welcome += "how to use the following commands\n --> command 255 255 255\n";
      welcome += "SetColorQuartile1 R G B : new color for quartil one\n";
      welcome += "SetColorQuartile2 R G B : new color for quartil two\n";
      welcome += "SetColorQuartile3 R G B : new color for quartil three\n";
      welcome += "SetColorQuartile4 R G B : new color for quartil four\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
    
  }
}

/* lightValue-->1;
 CO2Value-->2;
 tempValue-->3;
 humidityValue-->4;
 soilHumidityValue-->5;
 soilTemperatureValue-->6;
*/
void ring(int sensor)
{
  pixels.clear();
  switch(sensor){
    case 1:RING_LIGHT();
    break;
    case 2:RING_CO2();
    break;
    case 3:RING_TEMP();
    break;
    case 4:RING_HUM();
    break;
    case 5:RING_SOILHUM();
    break;
    case 6:RING_SOILTEM();
    break;
  }
}

void RING_LIGHT(){
  if(lightValue>800)
  {
      for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile4[0], quartile4[1], quartile4[2]));
      /*
      for(int i=0; i<3; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile1[0], quartile1[1], quartile1[2]));
      for(int i=3; i<6; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile2[0], quartile2[1], quartile2[2]));
      for(int i=6; i<9; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile3[0], quartile3[1], quartile3[2]));
      for(int i=9; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile4[0], quartile4[1], quartile4[2]));
        */
  }else if(lightValue>600)
  {
    for(int i=0; i<12; i++)  
      pixels.setPixelColor(i, pixels.Color(quartile3[0], quartile3[1], quartile3[2]));
    /*
    for(int i=0; i<3; i++)  
      pixels.setPixelColor(i, pixels.Color(quartile1[0], quartile1[1], quartile1[2]));
    for(int i=3; i<6; i++)  
      pixels.setPixelColor(i, pixels.Color(quartile2[0], quartile2[1], quartile2[2]));
    for(int i=6; i<9; i++)  
      pixels.setPixelColor(i, pixels.Color(quartile3[0], quartile3[1], quartile3[2]));
    */
  }else if(lightValue>400)
  {
    for(int i=0; i<12; i++)  
      pixels.setPixelColor(i, pixels.Color(quartile2[0], quartile2[1], quartile2[2]));
    /*
    for(int i=0; i<3; i++)  
      pixels.setPixelColor(i, pixels.Color(quartile1[0], quartile1[1], quartile1[2]));
    for(int i=3; i<6; i++)  
      pixels.setPixelColor(i, pixels.Color(quartile2[0], quartile2[1], quartile2[2]));
    */
  }else
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile1[0], quartile1[1], quartile1[2]));
  }
  pixels.show();
}

void RING_CO2(){
  if(CO2Value>800)
    {
      for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile4[0], quartile4[1], quartile4[2]));
  }else if(CO2Value>600)
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile3[0], quartile3[1], quartile3[2]));
  }else if(CO2Value>400)
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile2[0], quartile2[1], quartile2[2]));
  }else
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile1[0], quartile1[1], quartile1[2]));
  }
  pixels.show();
}

void RING_TEMP(){
  if(tempValue>800)
    {
      for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile4[0], quartile4[1], quartile4[2]));
  }else if(tempValue>600)
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile3[0], quartile3[1], quartile3[2]));
  }else if(tempValue>400)
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile2[0], quartile2[1], quartile2[2]));
  }else
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile1[0], quartile1[1], quartile1[2]));
  }
  pixels.show();
}

void RING_HUM(){
  if(humidityValue>800)
    {
      for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile4[0], quartile4[1], quartile4[2]));
  }else if(humidityValue>600)
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile3[0], quartile3[1], quartile3[2]));
  
  }else if(humidityValue>400)
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile2[0], quartile2[1], quartile2[2]));
  }else
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile1[0], quartile1[1], quartile1[2]));
  }
  pixels.show();
}

void RING_SOILHUM(){
  if(soilHumidityValue>800)
    {
      for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile4[0], quartile4[1], quartile4[2]));
  }else if(soilHumidityValue>600)
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile3[0], quartile3[1], quartile3[2]));
  }else if(soilHumidityValue>400)
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile2[0], quartile2[1], quartile2[2]));
  }else
  {
    for(int i=0; i<3; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile1[0], quartile1[1], quartile1[2]));
  }
  pixels.show();
}

void RING_SOILTEM(){
  if(soilTemperatureValue>800)
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile4[0], quartile4[1], quartile4[2]));
  }else if(soilTemperatureValue>600)
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile3[0], quartile3[1], quartile3[2]));
  }else if(soilTemperatureValue>400)
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile2[0], quartile2[1], quartile2[2]));

  }else
  {
    for(int i=0; i<12; i++)  
        pixels.setPixelColor(i, pixels.Color(quartile1[0], quartile1[1], quartile1[2]));
  }
            pixels.show();
}