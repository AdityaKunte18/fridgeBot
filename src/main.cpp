#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2SNoDAC *out;
AudioFileSourceID3 *id3;

const int sensorPin = A0;
const int threshold = 1023;
const long openTime = 3000;

long doorOpenStart = 0;
bool doorOpen = false;

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  Serial.printf("ID3 callback for: %s = '", type);

  if (isUnicode) {
    string += 2;
  }
  
  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
    Serial.printf("%c", a);
  }
  Serial.printf("'\n");
  Serial.flush();
}

void playAudio() {
  Serial.printf("Sample MP3 playback begins...\n");

  // Initialize random number generator
  randomSeed(millis());

  // Generate a random number between 1 and 8
  int randomNumber = random(1, 9); // Note: the upper limit is exclusive

  // Construct the filename
  char filename[10];
  sprintf(filename, "/%d.mp3", randomNumber);

  audioLogger = &Serial;
  file = new AudioFileSourceSPIFFS(filename); // Use the randomly chosen file
  id3 = new AudioFileSourceID3(file);
  id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);

  while (mp3->isRunning()) {
    if (!mp3->loop()) mp3->stop();
  }
  Serial.printf("MP3 done\n");
}

void setup() {
  WiFi.mode(WIFI_OFF); 
  Serial.begin(115200);
  SPIFFS.begin();
  Serial.println("setup done, now listening...");
}

void loop() {

  int sensorValue = analogRead(sensorPin);

  if (sensorValue < threshold && !doorOpen) {
    // Door has just been opened
    doorOpen = true;
    doorOpenStart = millis();
    Serial.println("door has just been opened");
  } else if (sensorValue >= threshold && doorOpen) {
    // Door has just been closed
    Serial.println("door has been closed");
    doorOpen = false;
  }

  if (doorOpen && millis() - doorOpenStart > openTime) {
    // Door has been open for more than the specified time
    Serial.println("Alert: Fridge door has been left open for too long!");
    playAudio(); // Call the playAudio function to start playback
    doorOpen = false; // Reset the door open status
  }

  delay(200);
}