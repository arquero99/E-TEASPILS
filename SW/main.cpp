#include <Arduino.h>
#include "credentials.h"
#include "configuracion.h"
#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include "ThingsBoard.h"
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_VEML7700.h"
#include <OneWire.h>
#include <DallasTemperature.h>

//Error Flags
bool eLEDSTRIPE=true;
bool eDISPLAY=true;
bool eSCD30=true;
bool eDS18B20=true;
bool eVEML7700=true;
bool eRTC=true;
bool eWIFI=true;
bool eTB=true;


// thingsboard
#define TOKEN tokenDeviceUCAEP
#define THINGSBOARD_SERVER thingsboardServer
//#define TOKEN_UPF tokenDevice_UPF
//#define THINGSBOARD_SERVER_UPF thingsboardServer_UPF
WiFiClient espClient;
ThingsBoard tb(espClient);
//ThingsBoard tbUPF(espClient);
int status = WL_IDLE_STATUS;

// telegram
#define BOT_TOKEN tokenBotUCAEP
const unsigned long BOT_MTBS = 1000; // mean time between scan messages
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime; // last time messages' scan has been done

// real time clock
RTC_DS3231 rtc;

// SD
File logData;
int hora = 0;

// SCD30 Sensor. CO2
SCD30 airSensor;

// veml7700. Light
Adafruit_VEML7700 veml = Adafruit_VEML7700();

// DS18B20. Soil Temperature
OneWire oneWireObject(SOILTEMP_SENSOR_PIN);
DallasTemperature soilTemperatureSensor(&oneWireObject);

// LED
Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// LED_RING
Adafruit_NeoPixel pixels(NUMPIXELS, RING_PIN, NEO_GRB + NEO_KHZ800);
int ringElement = 2;
int colorLow[] = {0, 240, 255};
int colorMedium[] = {0, 255, 121};
int colorLarge[] = {255, 195, 0};
int colorHigh[] = {255, 0, 0};

// OLED
Adafruit_SH1106 display(21, 22);

// Counter for data reading
int ambCont = 0;
int soilCont = 0;

// Period of data upload
int sendPeriod = DEFAULT_PERIOD;

int lightValue = 0;
int CO2Value = 0;
int tempValue = 0;
int humidityValue = 0;
int soilHumidityValue = 0;
int soilTemperatureValue = 0;
int soilTemperatureValue2 = 0;

int presentMoment = 0;
int displayMode = 1;
int fixedSensor = 1; // Inicialización arbitraria

void initDisplay();
void initRing();
void initCo2();
void initLight();
void initSoilTemp();
void initClock();
void checkSD();
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
String completeDate(int mode);
void conectIOT();
void sensorsRead();
void displayCarousel();
void displayAll();
void fixSensor(int sensor);
void logDataset();
void uploadData();
void checkBot();
void handleNewMessages(int numNewMessages);
void ring(int sensor);
void RING_LIGHT();
void RING_CO2();
void RING_TEMP();
void RING_HUM();
void RING_SOILHUM();
void RING_SOILTEM();
void noInternetMsg();
void firstRead();

void setup()
{
  Serial.begin(BAUD_RATE);
  initRing();
  initDisplay();

  delay(2000);

  checkSD(); // Se comprueba si hay microSD evitando continuar hasta que no se inserte. Espera Activa

  writeFile(SD, "/log.txt", LogInitialMessage.c_str());

  conectIOT(); // Se conecta a WIFI y la plataforma IoT

  initCo2();

  initSoilTemp();

  initLight();

  initClock();

  firstRead();
}

void loop()
{
  checkSD();
  if (WiFi.status() != WL_CONNECTED || !tb.connected())// || !tbUPF.connected())
  {
    conectIOT();
  }else{
    pixel.clear();
    pixel.show();
    Serial.println("Funciona");
  }

  checkBot();

  sensorsRead();

  if (displayMode == 3)
    displayCarousel();
  else if (displayMode == 1)
    displayAll();
  else
    fixSensor(fixedSensor);

  logDataset();

  if (presentMoment == sendPeriod)
  {
    uploadData();
    presentMoment = 0;
  }

  ambCont++;
  soilCont++;
  presentMoment++;

  delay(1000);
}

void initRing()
{

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();
  pixels.show();

  pixel.begin();
  pixel.clear();

  pixel.setPixelColor(0, pixel.Color(0, 0, 0));
  pixel.show();

  theaterChaseRainbow(50);

  pixels.clear();
  pixels.show();
  
    if ((int)pixels.getPixels()==12)
  {
    eLEDSTRIPE=false;
  }
}

void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<15; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      pixels.clear();         //   Set all pixels in RAM to 0 (off)
      for(int c=b; c<pixels.numPixels(); c += 3) {
        int      hue   = firstPixelHue + c * 65536L / pixels.numPixels();
        uint32_t color = pixels.gamma32(pixels.ColorHSV(hue)); // hue -> RGB
        pixels.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      pixels.show();               
      delay(wait);                 
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}

void initDisplay()
{
  //Check Display Connection 
  Wire.beginTransmission(60);
  int error = Wire.endTransmission();
  if (error==0)
  {
    eDISPLAY=false;
  }

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

  // miniature bitmap display
  display.clearDisplay();
  display.drawBitmap(0, 0, LOGO, 128, 64, 1);
  display.display();
  delay(3000);
}

void initCo2()
{
  Wire.begin();
  if (airSensor.begin() == false)
  {
    display.clearDisplay();
    display.setCursor(45, 24);
    //display.println(" No se ha ");
    display.setCursor(35, 32);
    display.println("Loading ...");
    display.setCursor(45, 40);
    //display.println("   SCD30  ");
    display.display();
    delay(3000);
  }
  else{
    eSCD30=false;
  }
}

void initSoilTemp()
{
  soilTemperatureSensor.begin();
  soilTemperatureSensor.requestTemperatures();

  if (soilTemperatureSensor.getTempCByIndex(0) == -127.00 || soilTemperatureSensor.getTempCByIndex(0) == 80.00)
  {
    soilTemperatureSensor.requestTemperatures();
    display.clearDisplay();
    display.setCursor(0, 24);
    display.println("The following sensor has not been detected");
    display.setCursor(45, 40);
    display.println("DS18B20");
    display.display();
    delay(3000);
    display.clearDisplay();
  }
  else{
    eDS18B20=false;
  }
}

void initLight()
{
  if (veml.begin() == false)
  {
    display.clearDisplay();
    display.setCursor(0, 24);
    display.println("The following sensor has not been detected");
    display.setCursor(45, 40);
    display.println("VEML7700");
    display.display();
    delay(3000);
    display.clearDisplay();
  }
  else{
    eVEML7700=false;
  }
  veml.setGain(VEML7700_GAIN_1);
  veml.setIntegrationTime(VEML7700_IT_800MS);
  veml.setLowThreshold(10000);
  veml.setHighThreshold(20000);
  veml.interruptEnable(true);
}

void initClock()
{
  if (!rtc.begin())
  {
    display.clearDisplay();
    display.setCursor(0, 24);
    display.println("The following component hasn't been detected");
    display.setCursor(45, 40);
    display.println("RTC Clock");
    display.display();
    delay(3000);
    display.clearDisplay();
  }
  else{
    eRTC=false;
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

void checkSD()
{
  SD.end();
  while (!SD.begin(5))
  {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("SD card not found");
    display.setCursor(0, 40);
    display.println("Please, insert a SD  card");
    display.display();
    pixel.setPixelColor(0, pixel.Color(255, 102, 204));
    pixel.show();
    delay(500);
    pixel.clear();
    delay(500);

  }
  // sdActiva = true;
}

// Writes data into SD card and prints a log on Serial console
void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char *path, const char *message)
{

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    return;
  }
  if (file.print(message))
  {
    // Serial.println("Message appended");
  }
  else
  {
    // Serial.println("Append failed");
  }
  file.close();
}

String completeDate(int mode)
{
  DateTime date = rtc.now();
  String formattedDate;
  if (mode == 2)
  {
    formattedDate = String(date.year()) + '/' + String(date.month()) + '/' + String(date.day()) + ' ' + String(date.hour()) + ':' + String(date.minute()) + ':' + String(date.second());
  }
  else
  {
    formattedDate = date.timestamp();
  }
  return formattedDate;
}

void conectIOT()
{
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  if (WiFi.status() != WL_CONNECTED)
  {

    for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++)
    {
      Serial.print(".");
      delay(750);
    }

    pixel.setPixelColor(0, pixel.Color(255, 255, 0));
    pixel.show();
    if (WiFi.status() != WL_CONNECTED)
      noInternetMsg();

    int i = 0;
    while (i < 5 && WiFi.status() != WL_CONNECTED)
    {
      i++;
      delay(500);
    }
  }
  else
  {
    eWIFI=false;
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
    if (!tb.connected())// || !tbUPF.connected())
    {
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN))// || !tbUPF.connect(THINGSBOARD_SERVER_UPF,TOKEN_UPF))
      {
        pixel.setPixelColor(0, pixel.Color(0, 0, 255));
        pixel.show();
        Serial.println("Connection to tb failed");
      }
      else
      {
        eTB=false;
        pixel.setPixelColor(0, pixel.Color(0, 0, 0));
        pixel.show();
        Serial.println("Connection to tb done");
      }
    }
  }
}

void sensorsRead()
{
  if (ambCont == ambPeriod)
  {
    lightValue = veml.readLux();
    CO2Value = airSensor.getCO2();
    tempValue = airSensor.getTemperature();
    humidityValue = airSensor.getHumidity();
    ambCont = 0;
  }

  if (soilCont == soilPeriod)
  {
    soilTemperatureSensor.requestTemperatures();
    soilHumidityValue = analogRead(SOILHUMIDITY_SENSOR_PIN);
    soilTemperatureValue = static_cast<int>(round(soilTemperatureSensor.getTempCByIndex(0)));
    soilTemperatureValue2 = static_cast<int>(round(soilTemperatureSensor.getTempCByIndex(1)));
    Serial.print("Valor sonda 1: ");
    Serial.println(soilTemperatureValue);
    Serial.print("Valor sonda 2: ");
    Serial.println(soilTemperatureValue2);
    soilCont = 0;
  }
}

void displayCarousel()
{
  ring(2);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(50, 0);
  display.print("CO2:");
  display.setCursor(0, 50);
  display.print(CO2Value);
  display.display();
  delay(cambio);

  ring(3);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Temperature:");
  display.setCursor(0, 50);
  display.print(tempValue);
  display.display();
  delay(cambio);

  ring(4);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(5, 0);
  display.print("Humidity:");
  display.setCursor(0, 50);
  display.print(humidityValue);
  display.display();
  delay(cambio);

  ring(1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(30, 0);
  display.print("Light:");
  display.setCursor(0, 50);
  display.print(lightValue);
  display.display();
  delay(cambio);

  ring(6);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(5, 0);
  display.print("Soil Temp:");
  display.setCursor(0, 50);
  display.print(soilTemperatureValue);
  display.display();
  delay(cambio);

  ring(5);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Soil Humidity:");
  display.setCursor(0, 50);
  display.print(soilHumidityValue);
  display.display();
  delay(cambio);
}

void displayAll()
{
  ring(ringElement);
  String isco2 = "";
  String istem = "";
  String ishum = "";
  String islig = "";
  String issoil1 = "";
  String issoil2 = "";

  switch (ringElement)
  {
  case 1:
    islig = "*";
    break;
  case 2:
    isco2 = "*";
    break;
  case 3:
    istem = "*";
    break;
  case 4:
    ishum = "*";
    break;
  case 5:
    issoil1 = "*";
    break;
  case 6:
    issoil2 = "*";
    break;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("CO2:" + isco2);
  display.setCursor(100, 0);
  display.println(CO2Value);

  display.setCursor(0, 10);
  display.print("Temperature:" + istem);
  display.setCursor(100, 10);
  display.print(tempValue);

  display.setCursor(0, 20);
  display.print("Humidity:" + ishum);
  display.setCursor(100, 20);
  display.print(humidityValue);

  display.setCursor(0, 30);
  display.print("Light:" + islig);
  display.setCursor(100, 30);
  display.print(lightValue);

  display.setCursor(0, 40);
  display.print("Soil Temp 1:" + issoil1);
  display.setCursor(100, 40);
  display.print(soilTemperatureValue);

  display.setCursor(0, 50);
  display.print("Soil Temp 2:" + issoil2);
  display.setCursor(100, 50);
  display.print(soilTemperatureValue2);

  display.display();
}

void fixSensor(int sensor)
{
  switch (sensor)
  {
  case 2:
  {
    ring(2);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("CO2:");
    display.setCursor(0, 28);
    display.print(CO2Value);
    display.display();
  }
  break;
  case 3:
  {
    ring(3);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("Temperature:");
    display.setCursor(0, 28);
    display.print(tempValue);
    display.display();
  }
  break;
  case 4:
  {
    ring(4);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("Humidity:");
    display.setCursor(0, 28);
    display.print(humidityValue);
    display.display();
  }
  break;
  case 1:
  {
    ring(1);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("Light:");
    display.setCursor(0, 28);
    display.print(lightValue);
    display.display();
  }
  break;
  case 6:
  {
    ring(6);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("Soil Temp:");
    display.setCursor(0, 28);
    display.print(soilTemperatureValue);
    display.display();
  }
  break;
  case 5:
  {
    ring(5);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("Soil Humidity:");
    display.setCursor(0, 28);
    display.print(soilHumidityValue);
    display.display();
  }
  break;
  }
}

void logDataset()
{
  //String writeLog = "\n" + completeDate(1) + ";" + CO2Value + ";" + humidityValue + ";" + lightValue + ";" + soilHumidityValue + ";" + soilTemperatureValue + ";" + tempValue + ";";
  String writeLog = "\n" + completeDate(1) + ";" + tempValue + ";" + CO2Value + ";" + humidityValue + ";" + lightValue + ";" + soilTemperatureValue + ";" + soilTemperatureValue2 + ";";

  if (!eLEDSTRIPE && !eDISPLAY && !eSCD30 && !eDS18B20 && !eVEML7700 && !eRTC && !eWIFI && !eTB)
  {
    writeLog.concat("0 ");
  }
  else{
    if (eLEDSTRIPE)
    {
      writeLog.concat("101 ");
    }
    if (eDISPLAY)
    {
      writeLog.concat("102 ");
    }
    if (eSCD30)
    {
      writeLog.concat("103 ");
    }
    if (eDS18B20)
    {
      writeLog.concat("104 ");
    }
    if (eVEML7700)
    {
      writeLog.concat("105 ");
    }
    if (eRTC)
    {
      writeLog.concat("106 ");
    }
    if (eWIFI)
    {
      writeLog.concat("200 ");
    }
    else if (eTB)
    {
      writeLog.concat("201 ");
    }
  }
  appendFile(SD, "/log.txt", writeLog.c_str());
}

void uploadData()
{
  Serial.println("ENVIO DATOS");
  tb.sendTelemetryInt("co2", CO2Value);
  tb.sendTelemetryInt("temperature", tempValue);
  tb.sendTelemetryInt("humidity", humidityValue);
  tb.sendTelemetryInt("light", lightValue);
  tb.sendTelemetryInt("soilTemp1", soilTemperatureValue);
  tb.sendTelemetryInt("soilTemp2", soilTemperatureValue2);

//  tbUPF.sendTelemetryInt("co2", CO2Value);
//  tbUPF.sendTelemetryInt("temperature", tempValue);
//  tbUPF.sendTelemetryInt("humidity", humidityValue);
//  tbUPF.sendTelemetryInt("light", lightValue);
//  tbUPF.sendTelemetryInt("soilTemp", soilTemperatureValue);
//  tbUPF.sendTelemetryInt("soilHumidity", soilHumidityValue);
}

void checkBot()
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
    // String text = bot.messages[i].text;
    String s = bot.messages[i].text;
    // Serial.println(s);
    String text;
    String param1;
    String param2;
    String param3;

    String delimiter = " ";
    size_t pos = 0;
    String token;
    if ((pos = s.indexOf(delimiter)) != -1)
    {
      token = s.substring(0, pos);
      text = token;
      // Serial.println(text);
      s.remove(0, pos + delimiter.length());
      if ((pos = s.indexOf(delimiter)) != -1)
      {
        token = s.substring(0, pos);
        param1 = token;
        s.remove(0, pos + delimiter.length());

        if ((pos = s.indexOf(delimiter)) != -1)
        {
          token = s.substring(0, pos);
          param2 = token;
          s.remove(0, pos + delimiter.length());
          token = s.substring(0);
          param3 = token;
          s.remove(0, pos + delimiter.length());
        }
      }
    }
    else
    {
      text = s;
    }

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    /*if (text == "/carousel")
    {
      displayMode = 3;
      bot.sendMessage(chat_id, "Carousel mode", "");
    }*/

    /*if (text == "/all")
    {
      displayMode = 1;
      bot.sendMessage(chat_id, "Screen showing all", "");
    }*/

    if (text == "/setlightbar")
    {
      ringElement = 1;
      bot.sendMessage(chat_id, "Bar shows light gradient", "");
    }

    if (text == "/setco2bar")
    {
      ringElement = 2;
      bot.sendMessage(chat_id, "Bar shows co2 gradient", "");
    }

    if (text == "/settempbar")
    {
      ringElement = 3;
      bot.sendMessage(chat_id, "Bar shows temperature gradient", "");
    }

    if (text == "/sethumiditybar")
    {
      ringElement = 4;
      bot.sendMessage(chat_id, "Bar shows humidity gradient", "");
    }

    if (text == "/ringsoiltemp1bar")
    {
      ringElement = 5;
      bot.sendMessage(chat_id, "Bar shows soil temperature 1 gradient", "");
    }

    if (text == "/setsoiltemp2bar")
    {
      ringElement = 6;
      bot.sendMessage(chat_id, "Bar shows soil temperature 2 gradient", "");
    }

    /*if (text == "/setlightbar")
    {
      displayMode = 2;
      fixedSensor = 1;
      bot.sendMessage(chat_id, "Display show now the light", "");
    }

    if (text == "/setco2bar")
    {
      displayMode = 2;
      fixedSensor = 2;
      bot.sendMessage(chat_id, "Display show now the co2", "");
    }

    if (text == "/settempbar")
    {
      displayMode = 2;
      fixedSensor = 3;
      bot.sendMessage(chat_id, "Display show now the temperature", "");
    }

    if (text == "/sethumiditybar")
    {
      displayMode = 2;
      fixedSensor = 4;
      bot.sendMessage(chat_id, "Display show now the humidity", "");
    }

    if (text == "/setsoiltemp1bar")
    {
      displayMode = 2;
      fixedSensor = 5;
      bot.sendMessage(chat_id, "Display show now the soil temperature 1", "");
    }

    if (text == "/setsoiltemp2bar")
    {
      displayMode = 2;
      fixedSensor = 6;
      bot.sendMessage(chat_id, "Display show now the soil temperature 2", "");
    }*/

    if (text == "/light")
    {
      String message = "Light: " + (String)lightValue;
      bot.sendMessage(chat_id, message, "");
    }
    if (text == "/co2")
    {
      String message = "CO2: " + (String)CO2Value;
      bot.sendMessage(chat_id, message, "");
    }
    if (text == "/temp")
    {
      String message = "Temperature: " + (String)tempValue;
      bot.sendMessage(chat_id, message, "");
    }
    if (text == "/humidity")
    {
      String message = "Humidity: " + (String)humidityValue;
      bot.sendMessage(chat_id, message, "");
    }
    if (text == "/soiltemp1")
    {
      String message = "Soil Temperature Probe 1: " + (String)soilTemperatureValue;
      bot.sendMessage(chat_id, message, "");
    }
    if (text == "/soiltemp2")
    {
      String message = "Soil Temperature Probe 2: " + (String)soilTemperatureValue2;
      bot.sendMessage(chat_id, message, "");
    }
    if (text == "/changecadence")
    {
      sendPeriod = param1.toInt();
      String message = "Period of data uploading setted to " + (String)sendPeriod + " secs";
      bot.sendMessage(chat_id, message, "");
    }

    /*if (text == "SetColorLow")
    {
      colorLow[0] = param1.toInt();
      colorLow[1] = param2.toInt();
      colorLow[2] = param3.toInt();
      bot.sendMessage(chat_id, "Updated the LOW color", "");
    }

    if (text == "SetColorMedium")
    {
      colorMedium[0] = param1.toInt();
      colorMedium[1] = param2.toInt();
      colorMedium[2] = param3.toInt();
      bot.sendMessage(chat_id, "Updated the MEDIUM color", "");
    }

    if (text == "SetColorLarge")
    {
      colorLarge[0] = param1.toInt();
      colorLarge[1] = param2.toInt();
      colorLarge[2] = param3.toInt();
      bot.sendMessage(chat_id, "Updated the LARGE color", "");
    }

    if (text == "SetColorHigh")
    {
      colorHigh[0] = param1.toInt();
      colorHigh[1] = param2.toInt();
      colorHigh[2] = param3.toInt();
      bot.sendMessage(chat_id, "Updated the VERY HIGH color", "");
    }*/
    /*if (text == "/COLOR")
    {
      String aux = (String)colorLow[0] + " " + (String)colorLow[1] + " " + (String)colorLow[2];
      bot.sendMessage(chat_id, aux, "");
    }*/
    if (text == "/start")
    {
      String welcome = "Welcome to TEASPILS chatbot, " + from_name + ".\n";
      //welcome += "/carousel : Show one by one\n";
      //welcome += "/all : Show all\n";
      welcome += "/setlightbar : Show light value in the bar\n";
      welcome += "/setco2bar : Show CO2 value in the bar\n";
      welcome += "/settempbar : Show temperature value in the bar\n";
      welcome += "/sethumiditybar : Show humidity value in the bar\n";
      welcome += "/setsoiltemp1bar : Show soil temperature probe 1 value in the bar\n";
      welcome += "/setsoiltemp2bar : Show soil temperature probe 2 value in the bar\n";
      welcome += "/light : Current light in lux\n";
      welcome += "/co2 : Current CO2 in ppm\n";
      welcome += "/temp : Current Celsius in degrees\n";
      welcome += "/humidity : Current humidity in percentage\n";
      welcome += "/soiltemp1 : Current soil temperature probe 1 in degrees\n";
      welcome += "/soiltemp2 : Current soil temperature probe 2 in degrees\n";
      //welcome += "/setlight : set light value in display\n";
      //welcome += "/setco2 : set CO2 value in display\n";
      //welcome += "/settemperature : set temperature value in display\n";
      //welcome += "/sethumidity : set humidity value in display\n";
      //welcome += "/setsoilHumidity : set soil humidity value in display\n";
      //welcome += "/setsoilTemperature : set soil temperature value in display\n";
      welcome += "/changecadence : Set sensor data reading frequency in seconds\n";
      /*welcome += "how to use the following commands\n --> command 255 255 255\n";
      welcome += "SetColorLow R G B : new color for low values\n";
      welcome += "SetColorMedium R G B : new color for medium values\n";
      welcome += "SetColorLarge R G B : new color for large values\n";
      welcome += "SetColorHigh R G B : new color for very high values\n";*/
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void ring(int sensor)
{
  pixels.clear();
  pixels.show();
  switch (sensor)
  {
  case 1:
    RING_LIGHT();
    break;
  case 2:
    RING_CO2();
    break;
  case 3:
    RING_TEMP();
    break;
  case 4:
    RING_HUM();
    break;
  case 5:
    RING_SOILTEM1();
    break;
  case 6:
    RING_SOILTEM2();
    break;
  }
}

void RING_LIGHT()
{
  if (lightValue > 300)
  {
    for (int i = NUMPIXELS-7; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLow[0], colorLow[1], colorLow[2]));
  }
  else if (lightValue > 200)
  {
    for (int i = NUMPIXELS-5; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
    
  }
  else if (lightValue > 100)
  {
    for (int i = NUMPIXELS-3; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else
  {
    for (int i = NUMPIXELS; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorHigh[0], colorHigh[1], colorHigh[2]));
  }
  pixels.show();
}

void RING_CO2()
{
  if (CO2Value > 1000)
  {
    for (int i = NUMPIXELS; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorHigh[0], colorHigh[1], colorHigh[2]));
  }
  else if (CO2Value > 600)
  {
    for (int i = NUMPIXELS-3; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else if (CO2Value > 400)
  {
    for (int i = NUMPIXELS-5; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else
  {
    for (int i = NUMPIXELS-7; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLow[0], colorLow[1], colorLow[2]));
  }
  pixels.show();
}

void RING_TEMP()
{
  if (tempValue > 30)
  {
    for (int i = NUMPIXELS; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorHigh[0], colorHigh[1], colorHigh[2]));
  }
  else if (tempValue > 25)
  {
    for (int i = NUMPIXELS-3; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else if (tempValue > 15)
  {
    for (int i = NUMPIXELS-2; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else
  {
    for (int i = NUMPIXELS-7; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLow[0], colorLow[1], colorLow[2]));
  }
  pixels.show();
}

void RING_HUM()
{
  if (humidityValue > 75)
  {
    for (int i = NUMPIXELS; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLow[0], colorLow[1], colorLow[2]));
  }
  else if (humidityValue > 50)
  {
    for (int i = NUMPIXELS-3; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else if (humidityValue > 25)
  {
    for (int i = NUMPIXELS-5; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else
  {
    for (int i = NUMPIXELS-7; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorHigh[0], colorHigh[1], colorHigh[2]));
  }
  pixels.show();
}

/*void RING_SOILHUM()
{
  if (soilHumidityValue > 60)
  {
    for (int i = 0; i < NUMPIXELS; i++)
      pixels.setPixelColor(i, pixels.Color(colorHigh[0], colorHigh[1], colorHigh[2]));
  }
  else if (soilHumidityValue > 40)
  {
    for (int i = 0; i < NUMPIXELS - 3; i++)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else if (soilHumidityValue > 30)
  {
    for (int i = 0; i < NUMPIXELS- 6; i++)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else
  {
    for (int i = 0; i < NUMPIXELS - 9; i++)
      pixels.setPixelColor(i, pixels.Color(colorLow[0], colorLow[1], colorLow[2]));
  }
  pixels.show();
}*/
//soilTemperatureValue
void RING_SOILTEM1()
{
  if (soilTemperatureValue > 30)
  {
    for (int i = NUMPIXELS; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorHigh[0], colorHigh[1], colorHigh[2]));
  }
  else if (soilTemperatureValue > 25)
  {
    for (int i = NUMPIXELS-3; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else if (soilTemperatureValue > 45)
  {
    for (int i = NUMPIXELS-5; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else
  {
    for (int i = NUMPIXELS-7; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLow[0], colorLow[1], colorLow[2]));
  }
  pixels.show();
}

void RING_SOILTEM2()
{
  if (soilTemperatureValue2 > 30)
  {
    for (int i = NUMPIXELS; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorHigh[0], colorHigh[1], colorHigh[2]));
  }
  else if (soilTemperatureValue2 > 25)
  {
    for (int i = NUMPIXELS-3; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else if (soilTemperatureValue2 > 15)
  {
    for (int i = NUMPIXELS-5; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else
  {
    for (int i = NUMPIXELS-7; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLow[0], colorLow[1], colorLow[2]));
  }
  pixels.show();
}


void noInternetMsg()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Internet Connection Failed ");
  display.setCursor(0, 16);
  display.println("Please, make sure that the following network is aviable");
  display.setCursor(50, 48);
  display.println(WIFI_NAME);
  display.display();
}

void firstRead()
{
  soilTemperatureSensor.requestTemperatures();

  lightValue = veml.readLux();
  CO2Value = airSensor.getCO2();
  tempValue = airSensor.getTemperature();
  humidityValue = airSensor.getHumidity();
  soilHumidityValue = analogRead(SOILHUMIDITY_SENSOR_PIN);
  soilTemperatureValue = static_cast<int>(round(soilTemperatureSensor.getTempCByIndex(0)));
}

/* lightValue-->1;
 CO2Value-->2;
 tempValue-->3;
 humidityValue-->4;
 soilHumidityValue-->5;
 soilTemperatureValue-->6;
*/