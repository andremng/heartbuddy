#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "heartRate.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OLED CONFIGURATION ---
// 128x32 pixels, I2C address 0x3C (most common; try 0x3D if display does not initialise).
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1  // No reset pin used; share the ESP32 reset line.
#define OLED_ADDRESS  0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MAX30105 particleSensor;

// --- SpO2 BUFFER ---
#define BUFFER_LENGTH 50
uint32_t irBuffer[BUFFER_LENGTH];
uint32_t redBuffer[BUFFER_LENGTH];

int32_t spo2Value      = 0;
int8_t  spo2Valid      = 0;
int32_t algoHeartRate  = 0;
int8_t  heartRateValid = 0;

// --- BPM VARIABLES ---
const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateSpot        = 0;
long lastBeat        = 0;
float beatsPerMinute = 0;
int beatAvg          = 0;
bool bufferFilled    = false;
byte validReadings   = 0;

// --- TIMING ---
unsigned long lastDisplayTime = 0;
unsigned long lastSpo2Time    = 0;
const int DISPLAY_INTERVAL    = 500;
const int SPO2_INTERVAL       = 2000;

const long FINGER_THRESHOLD   = 50000;

void resetState() {
  beatsPerMinute = 0;
  beatAvg        = 0;
  rateSpot       = 0;
  validReadings  = 0;
  bufferFilled   = false;
  spo2Value      = 0;
  spo2Valid      = 0;
  for (byte x = 0; x < RATE_SIZE; x++) rates[x] = 0;
}

// Clears the display and redraws all content in one function.
// Centralising all display logic here prevents flickering and keeps loop() clean.
void updateDisplay(long irValue) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (irValue < FINGER_THRESHOLD) {
    // Centred single message when no finger is detected.
    display.setTextSize(1);
    display.setCursor(10, 8);
    display.print("Place finger on");
    display.setCursor(28, 20);
    display.print("sensor...");

  } else if (!bufferFilled) {
    // Calibration progress shown as a fraction.
    display.setTextSize(1);
    display.setCursor(0, 4);
    display.print("Calibrating BPM:");
    display.setCursor(0, 18);
    display.print(validReadings);
    display.print("/");
    display.print(RATE_SIZE);
    display.print(" readings");

  } else {
    // Main readout: BPM on the top row, SpO2 on the bottom row.
    // Text size 2 gives 12x16 pixel characters, fitting 2 rows of 16 chars on 128x32.

    // Top row: BPM
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("BPM:");
    display.print(beatAvg);

    // Bottom row: SpO2
    display.setCursor(0, 17);
    display.print("SpO2:");
    if (spo2Valid) {
      display.print(spo2Value);
      display.print("%");
    } else {
      display.print("--");
    }
  }

  display.display(); // Push the buffer to the physical screen.
}

void setup() {
  Serial.begin(115200);

  // Initialise OLED before the sensor so any startup error is visible on screen.
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found. Check wiring and I2C address.");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 12);
  display.print("Initializing...");
  display.display();

  Serial.println("Initializing MAX30102...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found. Check wiring and power supply.");
    display.clearDisplay();
    display.setCursor(0, 8);
    display.print("MAX30102 error!");
    display.setCursor(0, 20);
    display.print("Check wiring.");
    display.display();
    while (1);
  }

  byte ledBrightness = 60;
  byte sampleAverage = 1;
  byte ledMode       = 2;
  int  sampleRate    = 400;
  int  pulseWidth    = 215;
  int  adcRange      = 4096;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

  display.clearDisplay();
  display.setCursor(10, 12);
  display.print("Ready. Place");
  display.setCursor(22, 22);
  display.print("finger...");
  display.display();
}

void loop() {

  long irValue = particleSensor.getIR();

  // --- FINGER DETECTION ---
  if (irValue < FINGER_THRESHOLD) {
    resetState();
    if (millis() - lastDisplayTime >= DISPLAY_INTERVAL) {
      Serial.print("IR: ");
      Serial.print(irValue);
      Serial.println(" | No finger detected.");
      updateDisplay(irValue);
      lastDisplayTime = millis();
    }
    return;
  }

  // --- CONTINUOUS BPM ---
  if (checkForBeat(irValue)) {
    long delta     = millis() - lastBeat;
    lastBeat       = millis();
    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute > 40 && beatsPerMinute < 160) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      if (!bufferFilled) {
        validReadings++;
        if (validReadings >= RATE_SIZE) bufferFilled = true;
      }

      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // --- PERIODIC SpO2 ---
  if (millis() - lastSpo2Time >= SPO2_INTERVAL) {
    for (byte i = 0; i < BUFFER_LENGTH; i++) {
      while (particleSensor.available() == false) {
        particleSensor.check();
      }
      redBuffer[i] = particleSensor.getRed();
      irBuffer[i]  = particleSensor.getIR();
      particleSensor.nextSample();
    }

    maxim_heart_rate_and_oxygen_saturation(
      irBuffer, BUFFER_LENGTH, redBuffer,
      &spo2Value, &spo2Valid,
      &algoHeartRate, &heartRateValid
    );

    if (spo2Value < 70 || spo2Value > 100) spo2Valid = 0;

    lastSpo2Time = millis();
  }

  // --- SERIAL AND DISPLAY OUTPUT ---
  if (millis() - lastDisplayTime >= DISPLAY_INTERVAL) {

    Serial.print("IR: ");
    Serial.print(irValue);
    Serial.print(" | BPM: ");
    if (bufferFilled) {
      Serial.print(beatAvg);
    } else {
      Serial.print("Calibrating (");
      Serial.print(validReadings);
      Serial.print("/");
      Serial.print(RATE_SIZE);
      Serial.print(")");
    }
    Serial.print(" | SpO2: ");
    if (spo2Valid) {
      Serial.print(spo2Value);
      Serial.print("%");
    } else {
      Serial.print("-- (keep finger still)");
    }
    Serial.println();

    updateDisplay(irValue);
    lastDisplayTime = millis();
  }
}