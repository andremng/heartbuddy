// Main file running setup and loop functions and the overall logic of the system
//-------------------------------------------------------------------------------

#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "heartRate.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// --- OLED CONFIGURATION ---
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

// --- SpO2 BUFFER ---
#define BUFFER_LENGTH 50

// --- BPM CONFIGURATION ---
const byte RATE_SIZE = 8;

// --- TIMING ---
const int DISPLAY_INTERVAL = 500;
const int SPO2_INTERVAL    = 2000;

// --- SENSOR THRESHOLD ---
const long FINGER_THRESHOLD = 50000;

// --- GLOBAL OBJECTS ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MAX30105 particleSensor;

// --- SpO2 VARIABLES ---
uint32_t irBuffer[BUFFER_LENGTH];
uint32_t redBuffer[BUFFER_LENGTH];

int32_t spo2Value      = 0;
int8_t  spo2Valid      = 0;
int32_t algoHeartRate  = 0;
int8_t  heartRateValid = 0;

// --- BPM VARIABLES ---
byte rates[RATE_SIZE];
byte rateSpot        = 0;
long lastBeat        = 0;
float beatsPerMinute = 0;
int beatAvg          = 0;
bool bufferFilled    = false;
byte validReadings   = 0;

// --- TIMING VARIABLES ---
unsigned long lastDisplayTime = 0;
unsigned long lastSpo2Time    = 0;

void setup() {
  Serial.begin(115200);

  initDisplay();
  showStartupMessage();

  initSensor();
  showReadyMessage();
  initNetwork();
}

void loop() {
  long irValue = particleSensor.getIR();

  if (!isFingerDetected(irValue)) {
    handleNoFinger(irValue);
    return;
  }

  updateBPM(irValue);
  updateSpo2IfNeeded();
  updateOutputIfNeeded(irValue);
  sendReadingIfNeeded();

}