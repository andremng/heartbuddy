#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "heartRate.h"

MAX30105 particleSensor;

// --- SpO2 BUFFER CONFIGURATION ---
// Reduced from 100 to 50 samples to halve the collection window.
// At 400 Hz with no hardware averaging, 50 samples = ~0.5 seconds per SpO2 update.
#define BUFFER_LENGTH 50

uint32_t irBuffer[BUFFER_LENGTH];
uint32_t redBuffer[BUFFER_LENGTH];

int32_t spo2Value      = 0;
int8_t  spo2Valid      = 0;
int32_t algoHeartRate  = 0; // Kept only to satisfy the algorithm signature, not displayed.
int8_t  heartRateValid = 0;

// --- BPM VARIABLES (peak detection, runs continuously) ---
const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateSpot        = 0;
long lastBeat        = 0;
float beatsPerMinute = 0;
int beatAvg          = 0;
bool bufferFilled    = false;
byte validReadings   = 0;

// --- TIMING ---
unsigned long lastPrintTime   = 0;
unsigned long lastSpo2Time    = 0;
const int PRINT_INTERVAL      = 500;
const int SPO2_INTERVAL       = 2000; // Recalculate SpO2 every 2 seconds.

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

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing MAX30102...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found. Check wiring and power supply.");
    while (1);
  }

  // Key changes from previous version:
  // - sampleAverage = 1: no hardware averaging, maximum responsiveness.
  // - ledMode = 2: Red + IR both active, required for SpO2.
  // - sampleRate = 400: four times faster than before, fills SpO2 buffer in ~0.5s.
  // - ledBrightness = 60: mid-range value, adjust upward (max 255) if IR reads below 50000.
  byte ledBrightness = 60;
  byte sampleAverage = 1;
  byte ledMode       = 2;
  int  sampleRate    = 400;
  int  pulseWidth    = 215;  // Shorter pulse width is compatible with 400Hz sample rate.
  int  adcRange      = 4096;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
}

void loop() {

  long irValue = particleSensor.getIR();

  // --- FINGER DETECTION ---
  if (irValue < FINGER_THRESHOLD) {
    resetState();
    if (millis() - lastPrintTime >= PRINT_INTERVAL) {
      Serial.print("IR: ");
      Serial.print(irValue);
      Serial.println(" | No finger detected.");
      lastPrintTime = millis();
    }
    return;
  }

  // --- CONTINUOUS BPM: peak detection on every sample ---
  if (checkForBeat(irValue)) {
    long delta      = millis() - lastBeat;
    lastBeat        = millis();
    beatsPerMinute  = 60 / (delta / 1000.0);

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

  // --- PERIODIC SpO2: batch collection every SPO2_INTERVAL milliseconds ---
  if (millis() - lastSpo2Time >= SPO2_INTERVAL) {

    // Collect a fresh buffer of 50 samples for the SpO2 algorithm.
    // At 400Hz this takes approximately 125 milliseconds.
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

    // Sanity check: discard SpO2 values outside the physiologically possible range.
    if (spo2Value < 70 || spo2Value > 100) {
      spo2Valid = 0;
    }

    lastSpo2Time = millis();
  }

  // --- SERIAL OUTPUT ---
  if (millis() - lastPrintTime >= PRINT_INTERVAL) {

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
    lastPrintTime = millis();
  }
}