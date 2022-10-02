#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

File logData;
void checkSD();
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);

void setup() {
  Serial.begin(9600);
  checkSD();
  writeFile(SD, "/log.txt", "Prueba");
  // put your setup code here, to run once:
}

void loop() {
  appendFile(SD, "/log.txt", " Hola");
  delay(1000);
}

void checkSD()
{
  SD.end();
  while (!SD.begin())
  {
    Serial.println("SD NO ENCONTRADA");
    delay(500);

  }
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
     Serial.println("Message appended");
  }
  else
  {
    // Serial.println("Append failed");
  }
  file.close();
} 