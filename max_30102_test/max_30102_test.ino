#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

// FIX 1: Increased buffer size from 4 to 8 for a significantly smoother average.
// A larger buffer means individual noisy beats have less weight on the final result.
const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;

// FIX 2: Added a validity flag to track whether the buffer has been fully populated.
// Before the buffer is full, beatAvg is diluted by the initial zeros and should not be displayed.
bool bufferFilled = false;
byte validReadings = 0;

unsigned long lastPrintTime = 0;
const int PRINT_INTERVAL = 500;

const long FINGER_THRESHOLD = 50000;

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing MAX30102...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 was not found. Please check wiring and power.");
    while (1);
  }

  // Slightly more conservative setup: lower sample rate and higher pulse width
  // improve signal quality and reduce noise-triggered false beat detections.
  particleSensor.setup(60, 4, 2, 100, 411, 4096);
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
}

void loop() {
  long irValue = particleSensor.getIR();

  if (irValue > FINGER_THRESHOLD) {

    if (checkForBeat(irValue) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute > 40 && beatsPerMinute < 160) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        // FIX 3: Track how many valid readings have been stored so far.
        // Only mark the buffer as filled once all 8 slots contain real data.
        if (!bufferFilled) {
          validReadings++;
          if (validReadings >= RATE_SIZE) {
            bufferFilled = true;
          }
        }

        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) {
          beatAvg += rates[x];
        }
        beatAvg /= RATE_SIZE;
      }
    }

  } else {
    // FIX 4: On finger removal, fully reset the buffer array and all state.
    // This prevents stale readings from corrupting the first average after re-contact.
    beatsPerMinute = 0;
    beatAvg = 0;
    rateSpot = 0;
    validReadings = 0;
    bufferFilled = false;
    for (byte x = 0; x < RATE_SIZE; x++) {
      rates[x] = 0;
    }
  }

  if (millis() - lastPrintTime >= PRINT_INTERVAL) {

    if (irValue < FINGER_THRESHOLD) {
      Serial.print("IR: ");
      Serial.print(irValue);
      Serial.println(" | No finger detected.");

    } else if (!bufferFilled) {
      // FIX 5: Inform the user that the sensor is still warming up its buffer.
      // This avoids displaying meaningless low averages during the first few beats.
      Serial.print("IR: ");
      Serial.print(irValue);
      Serial.print(" | Calibrating... (");
      Serial.print(validReadings);
      Serial.print("/");
      Serial.print(RATE_SIZE);
      Serial.println(" readings collected)");

    } else {
      // FIX 6: Display only beatAvg, not the raw instantaneous beatsPerMinute.
      // The instantaneous value is inherently noisy and should only be used internally.
      Serial.print("IR: ");
      Serial.print(irValue);
      Serial.print(" | Avg BPM: ");
      Serial.println(beatAvg);
    }

    lastPrintTime = millis();
  }
}