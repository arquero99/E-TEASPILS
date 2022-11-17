#include <Arduino.h>
#include "credentials.h"
#include "configuration.h"
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

// Error Flags
bool eLEDSTRIPE = true;
bool eDISPLAY = true;
bool eSCD30 = true;
bool eDS18B20 = true;
bool eVEML7700 = true;
bool eRTC = true;
bool eWIFI = true;
bool eTB = true;
bool eTG = true;
bool eSD = true;
bool eCON = true;

// thingsboard
WiFiClient espClient;
ThingsBoard tb(espClient);
int status = WL_IDLE_STATUS;

// telegram
#define BOT_TOKEN tokenBot
const unsigned long BOT_MTBS = 1000; // mean time between scan messages
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime; // last time messages' scan has been done

// real time clock
RTC_DS3231 rtc;
const char *ntpServer = "pool.ntp.org";

// SD
File logData;
int hora = 0;
String path;
String file_number;

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
int colorMedium[] = {0, 255, 0};
int colorLarge[] = {255, 195, 0};
int colorHigh[] = {255, 0, 0};

// OLED
Adafruit_SH1106 display(21, 22);

// Period of data upload
int sendPeriod = DEFAULT_PERIOD;

int lightValue = 0;
int CO2Value = 0;
int tempValue = 0;
int humidityValue = 0;
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
bool init_IoT();
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
void RING_SOILTEM1();
void RING_SOILTEM2();
void noInternetMsg();
void firstRead();
void theaterChaseRainbow(int wait);
void updateTime();
void init_tb();
void init_tg();
bool init_WIFI();
void internetMsg();

void setup()
{
  Serial.begin(BAUD_RATE);

  initRing();
  initDisplay();
  initClock();
  if (init_IoT()) // Se conecta a WIFI y la plataforma IoT
  {
    updateTime();
  }

  checkSD(); // Se comprueba si hay microSD evitando continuar hasta que no se inserte. Espera Activa
  if (!eSD)
  {
    char format[] = "YYYY-MM-DD-hh-mm";
    file_number = rtc.now().toString(format);
    path = "/log_";
    path.concat(file_number);
    path.concat(".csv");
    writeFile(SD, path.c_str(), LogInitialMessage.c_str());
    Serial.println("Escribiendo datos en SD en ruta: ");
    Serial.print(path);
  }

  initCo2();

  initSoilTemp();

  initLight();

  firstRead();
}

void loop()
{
  Serial.println("-----------------");
  DateTime exTime=rtc.now();
  Serial.println(exTime.timestamp());
  Serial.print("Iterator: ");
  Serial.println(presentMoment);
  checkSD();

  if (!eTG)
  {
    checkBot();
  }

  sensorsRead();
  delay(2000);

  if (displayMode == 3)
    displayCarousel();
  else if (displayMode == 1)
    displayAll();
  else
    fixSensor(fixedSensor);

  logDataset();

  if (presentMoment == 0)
  {
    if(!eTB){
            uploadData();
    }
    presentMoment = DEFAULT_PERIOD;
  }

  presentMoment--;
}

void initRing()
{
    Serial.println("*************************************");
  Serial.println("Inicializando Stripe");
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  theaterChaseRainbow(50);
  pixels.clear();

  if ((int)pixels.numPixels() == 8)
  {
    eLEDSTRIPE = false;
    Serial.println("Stripe con 8 LEDS");
  }
  else
  {
    int leds = pixels.numPixels();
    Serial.println("Stripe defectuoso ");
    Serial.print(leds);
  }
}

void theaterChaseRainbow(int wait)
{
  int firstPixelHue = 0; // First pixel starts at red (hue 0)
  for (int a = 0; a < 15; a++)
  { // Repeat 30 times...
    for (int b = 0; b < 3; b++)
    {                 //  'b' counts from 0 to 2...
      pixels.clear(); //   Set all pixels in RAM to 0 (off)
      for (int c = b; c < pixels.numPixels(); c += 3)
      {
        int hue = firstPixelHue + c * 65536L / pixels.numPixels();
        uint32_t color = pixels.gamma32(pixels.ColorHSV(hue)); // hue -> RGB
        pixels.setPixelColor(c, color);                        // Set pixel 'c' to value 'color'
      }
      pixels.show();
      delay(wait);
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}

void initDisplay()
{
  bool check = CHECK_OLED;
  if (check)
  {
    Serial.println("Inicialización de oled ");
    int i = 0;
    Wire.beginTransmission(60);
    int error = Wire.endTransmission();
    if (error == 0)
    {
      eDISPLAY = false;
    }
    else
    {
      while (i < 10 && error != 0)
      {
        display.begin(SH1106_SWITCHCAPVCC, 0x3C);
        Wire.beginTransmission(60);
        error = Wire.endTransmission();
        delay(100);
        Serial.println("Inicializando OLED");
        Serial.println(error);
        i++;
      }
      if (error != 0)
        eDISPLAY = true;
      else
        eDISPLAY = false;
    }
    if (!eDISPLAY)
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
    }
  }
  else
  {
    display.begin(SH1106_SWITCHCAPVCC, 0x3C);
    Serial.println("Inicializando OLED");
    eDISPLAY = false;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
  }
  Serial.println("Mensaje de bienvenida");
  display.setCursor(45, 24);
  display.println("Starting");
  display.setCursor(25, 32);
  display.println("Teaspils system");
  display.setCursor(112, 56);
  display.println("V3");
  display.setCursor(0, 56);
  display.println(UNIVERSITY);
  display.display();
  delay(1000);
  display.clearDisplay();
  delay(200);
  display.drawBitmap(0, 0, LOGO, 128, 64, WHITE);
  display.display();
  delay(3000);
  pixel.clear();
  display.clearDisplay();
}

void initCo2()
{
  Serial.println("Inicializacion SCD30");
  Wire.begin();
  if (airSensor.begin() == false)
  {
    Serial.println("Fallo en SCD30");
    display.clearDisplay();
    display.setCursor(0, 24);
    display.println("The following sensor has not been detected");
    display.setCursor(45, 40);
    display.println("SCD30");
    display.display();
    delay(3000);
  }
  else
  {
    Serial.println("SCD30 INICIALIZADO");
    eSCD30 = false;
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
  else
  {
    eDS18B20 = false;
  }
}

void initLight()
{
  int i = 0;
  Serial.println("Inicializacion VEML");
  while (!veml.begin() && i < 3)
  {
    display.clearDisplay();
    display.setCursor(0, 24);
    display.println("The following sensor has not been detected");
    display.setCursor(45, 40);
    display.println("VEML7700");
    display.display();
    delay(1000);
    display.clearDisplay();
    i++;
  }
  eVEML7700 = !veml.begin();
  if (!eVEML7700)
  {
    Serial.println("VEML INICIALIZADO");
  }
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
    eRTC = true;
    delay(3000);
    display.clearDisplay();
  }
  else
  {
    eRTC = false;
  }
  /*
  // Si se ha perdido la corriente, fijar fecha y hora
  if (rtc.lostPower())
  {
    // Fijar a fecha y hora de compilacion
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Fijar a fecha y hora específica. En el ejemplo, 21 de Enero de 2016 a las 03:00:00
    // rtc.adjust(DateTime(2016, 1, 21, 3, 0, 0));
  }
  */
}

void updateTime()
{
  configTime(GMT_OFFSET, GMT_DST, ntpServer);
  DateTime newDate;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Fallo al obtener la hora");
    return;
  }
  else
  {
    int year,month,day,hour,minute,second;
    year=1900+timeinfo.tm_year;
    month=timeinfo.tm_mon+1;
    day=timeinfo.tm_mday;
    hour=timeinfo.tm_hour;
    minute=timeinfo.tm_min;
    second=timeinfo.tm_sec;
    Serial.println("Actualizada a la hora NTP: ");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    rtc.adjust(DateTime(year,month,day,hour,minute,second));

    newDate = rtc.now();
    Serial.println("Hora en reloj: ");
    Serial.print("Date : ");
    Serial.print(newDate.day());
    Serial.print("/");
    Serial.print(newDate.month());
    Serial.print("/");
    Serial.print(newDate.year());
    Serial.print("\t Hour : ");
    Serial.print(newDate.hour());
    Serial.print(":");
    Serial.print(newDate.minute());
    Serial.print(":");
    Serial.println(newDate.second());
  }
}

void checkSD()
{
  Serial.println("inicialización de SD ");
  int i = 0;
  SD.end();
  while (!SD.begin() && i < 10)
  {
    Serial.println("SD NO ENCONTRADA");
    delay(500);
    i++;
  }
  if (i >= 10)
  {
    eSD = true;
    Serial.println("SD NO ENCONTRADA");
    pixel.setPixelColor(0, pixel.Color(255, 0, 0));
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("SD card not found");
    display.setCursor(0, 40);
    display.println("Please, insert a SD  card");
    pixel.show();
    display.display();
    delay(2000);
    pixel.clear();
  }
  else
  {
    eSD = false;
    Serial.println("SD ENCONTRADA");
  }
}

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
 
bool init_IoT()
{
  bool conection = false;
  Serial.println("Inicialización de IoT ");
  if (init_WIFI())
  {
    init_tb();
    init_tg();
    conection = true;
    eTB = false;
    eTG = false;
  }
  else
  {
    noInternetMsg();
    eTB = true;
    eTG = true;
  }
  return conection;
}

bool init_WIFI()
{
  Serial.println("Conexion de wifi");
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WIFI");
    for (int i = 0; i < 80 && WiFi.status() != WL_CONNECTED; i++)
    {
      delay(750);
      Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED)
    {
      eWIFI = true;
    }
    else
    {
      eWIFI = false;
      internetMsg();
    }
  }
  else
    eWIFI = false;
  return !eWIFI;
}

void init_tg()
{
  Serial.println("Inicialización de telegram ");
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  // Add root certificate for api.telegram.org
}

void init_tb()
{
  Serial.println("Inicialización de thingsboard ");
  if (!tb.connect(THINGSBOARD_SERVER, TOKEN))
  {
    eTB = true;
    Serial.println("Connection to tb failed");
  }
  else
  {
    eTB = false;
    Serial.println("Connection to tb done");
  }
}

void sensorsRead()
{
  lightValue = veml.readLux(VEML_LUX_AUTO);
  Serial.print("Light: ");
  Serial.println(lightValue);

  if (airSensor.dataAvailable())
  {
    CO2Value = airSensor.getCO2();
    Serial.print("CO2: ");
    Serial.println(CO2Value);

    tempValue = airSensor.getTemperature();
    Serial.print("Temperature: ");
    Serial.println(tempValue);

    humidityValue = airSensor.getHumidity();
    Serial.print("Humidity: ");
    Serial.println(humidityValue);
  }

  soilTemperatureSensor.requestTemperatures();
  soilTemperatureValue = static_cast<int>(round(soilTemperatureSensor.getTempCByIndex(0)));
  Serial.print("Probe 1: ");
  Serial.println(soilTemperatureValue);
  soilTemperatureValue2 = static_cast<int>(round(soilTemperatureSensor.getTempCByIndex(1)));
  Serial.print("Probe 2: ");
  Serial.println(soilTemperatureValue2);
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
  display.print("Soil Temp 2:");
  display.setCursor(0, 50);
  display.print(soilTemperatureValue2);
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
    display.print("Soil Temp 2:");
    display.setCursor(0, 28);
    display.print(soilTemperatureValue2);
    display.display();
  }
  break;
  }
}

void logDataset()
{
  String writeLog = "\n" + completeDate(1) + ";" + CO2Value + ";" + humidityValue + ";" + tempValue + ";" + lightValue + ";" + soilTemperatureValue + ";" + soilTemperatureValue2;

  if (!eLEDSTRIPE && !eDISPLAY && !eSCD30 && !eDS18B20 && !eVEML7700 && !eRTC && !eWIFI && !eTB)
  {
    writeLog.concat(";0");
  }
  else
  {
    writeLog.concat(";");
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
  appendFile(SD, path.c_str(), writeLog.c_str());
}

void uploadData()
{
  int i=0;
  Serial.println("ENVIO DATOS");
  while(!tb.sendTelemetryInt("co2", CO2Value)&&i<5){
    Serial.print("Error al enviar CO2");
    delay(200);
    i++;
  }
  i=0;
  while(!tb.sendTelemetryFloat("temperature", tempValue)&&i<5){
    Serial.print("Error al enviar Temp");
    delay(200);
    i++;
  }
  i=0;
  while(!tb.sendTelemetryFloat("humidity", humidityValue)&&i<5){
    Serial.print("Error al enviar Humidity");
    delay(200);
    i++;
  }
  i=0;
  while(!tb.sendTelemetryInt("light", lightValue)&&i<5){
    Serial.print("Error al enviar Lux");
    delay(200);
    i++;
  }
  i=0;
  while(!tb.sendTelemetryFloat("soilTemp1", soilTemperatureValue)&&i<5){
    Serial.print("Error al enviar Temp1");
    delay(200);
    i++;
  }
  i=0;
  while(!tb.sendTelemetryFloat("soilTemp2", soilTemperatureValue2)&&i<5){
    Serial.print("Error al enviar Temp2");
    delay(200);
    i++;
  }
  i=0;
  Serial.println("DATOS ENVIADOS");
}

void checkBot()
{
  Serial.println("Checking BOT");
  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    Serial.println("Getting messages");
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
    String s = bot.messages[i].text;
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
    if (text == "/start")
    {
      String welcome = "Welcome to TEASPILS chatbot, " + from_name + ".\n";
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
      welcome += "/changecadence : Set sensor data reading frequency in seconds\n";
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
    for (int i = NUMPIXELS - 7; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLow[0], colorLow[1], colorLow[2]));
  }
  else if (lightValue > 200)
  {
    for (int i = NUMPIXELS - 5; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else if (lightValue > 100)
  {
    for (int i = NUMPIXELS - 3; i >= 0; i--)
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
    for (int i = NUMPIXELS - 3; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else if (CO2Value > 400)
  {
    for (int i = NUMPIXELS - 5; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else
  {
    for (int i = NUMPIXELS - 7; i >= 0; i--)
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
    for (int i = NUMPIXELS - 3; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else if (tempValue > 15)
  {
    for (int i = NUMPIXELS - 2; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else
  {
    for (int i = NUMPIXELS - 7; i >= 0; i--)
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
    for (int i = NUMPIXELS - 3; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else if (humidityValue > 25)
  {
    for (int i = NUMPIXELS - 5; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else
  {
    for (int i = NUMPIXELS - 7; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorHigh[0], colorHigh[1], colorHigh[2]));
  }
  pixels.show();
}

void RING_SOILTEM1()
{
  if (soilTemperatureValue > 30)
  {
    for (int i = NUMPIXELS; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorHigh[0], colorHigh[1], colorHigh[2]));
  }
  else if (soilTemperatureValue > 25)
  {
    for (int i = NUMPIXELS - 3; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else if (soilTemperatureValue > 45)
  {
    for (int i = NUMPIXELS - 5; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else
  {
    for (int i = NUMPIXELS - 7; i >= 0; i--)
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
    for (int i = NUMPIXELS - 3; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLarge[0], colorLarge[1], colorLarge[2]));
  }
  else if (soilTemperatureValue2 > 15)
  {
    for (int i = NUMPIXELS - 5; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorMedium[0], colorMedium[1], colorMedium[2]));
  }
  else
  {
    for (int i = NUMPIXELS - 7; i >= 0; i--)
      pixels.setPixelColor(i, pixels.Color(colorLow[0], colorLow[1], colorLow[2]));
  }
  pixels.show();
}

void noInternetMsg()
{
  Serial.println("Conexion WIFI Fallida");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Internet Connection Failed ");
  display.setCursor(0, 16);
  display.println("Please, make sure that the following network is aviable");
  display.setCursor(24, 48);
  display.println(WIFI_NAME);
  display.display();
  for (int i = 0; i < 2; i++)
  {
    pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // RED
    pixel.show();
    delay(500);
    pixel.clear();
    delay(500);
  }
  display.clearDisplay();
}

void internetMsg()
{
  Serial.println("Conexion WIFI Correcta");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Internet Connection");
  display.setCursor(58, 16);
  display.print("OK");
  display.setCursor(0, 32);
  display.println("Name of the Network");
  display.setCursor(24, 48);
  display.println(WIFI_NAME);
  display.display();
  for (int i = 0; i < 2; i++)
  {
    pixel.setPixelColor(0, pixel.Color(0, 255, 0)); // GREEN
    pixel.show();
    delay(500);
    pixel.clear();
    delay(500);
  }
  display.clearDisplay();
}

void firstRead()
{
  soilTemperatureSensor.requestTemperatures();

  lightValue = veml.readLux();
  CO2Value = airSensor.getCO2();
  tempValue = airSensor.getTemperature();
  humidityValue = airSensor.getHumidity();
  soilTemperatureValue = static_cast<int>(round(soilTemperatureSensor.getTempCByIndex(0)));
}