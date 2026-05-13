// This file contains the functions driving the MAX30102 sensor
//-------------------------------------------------------------

// Initializing OLED display, showing startup messages on screen
void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found. Check wiring and I2C address.");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void showStartupMessage() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 12);
  display.print("Initializing...");
  display.display();
}

void showReadyMessage() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 12);
  display.print("Ready. Place");
  display.setCursor(22, 22);
  display.print("finger...");
  display.display();
}

// Initializing Sensor, printing error messagges if occured
void initSensor() {
  Serial.println("Initializing MAX30102...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found. Check wiring and power supply.");

    display.clearDisplay();
    display.setTextSize(1);
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

  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    sampleRate,
    pulseWidth,
    adcRange
  );
}

// Return ir Value if finger is deteced on sensor
bool isFingerDetected(long irValue) {
  return irValue >= FINGER_THRESHOLD;
}

// Reset the values if finger is removed
void resetSensorState() {
  beatsPerMinute = 0;
  beatAvg        = 0;
  rateSpot       = 0;
  validReadings  = 0;
  bufferFilled   = false;
  spo2Value      = 0;
  spo2Valid      = 0;

  for (byte x = 0; x < RATE_SIZE; x++) {
    rates[x] = 0;
  }
}

// Printing messages if finger is not deteced
void handleNoFinger(long irValue) {
  resetSensorState();

  if (millis() - lastDisplayTime >= DISPLAY_INTERVAL) {
    Serial.print("IR: ");
    Serial.print(irValue);
    Serial.println(" | No finger detected.");

    updateDisplay(irValue);
    lastDisplayTime = millis();
  }
}

// Calculates BPM value based on peak detection, updates buffer with 8 readings, does the average
void updateBPM(long irValue) {
  if (checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute > 40 && beatsPerMinute < 160) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

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
}

// Calculates SP02 values and updates buffers
void updateSpo2IfNeeded() {
  if (millis() - lastSpo2Time < SPO2_INTERVAL) {
    return;
  }

  for (byte i = 0; i < BUFFER_LENGTH; i++) {
    while (particleSensor.available() == false) {
      particleSensor.check();
    }

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();

    particleSensor.nextSample();
  }

  maxim_heart_rate_and_oxygen_saturation(
    irBuffer,
    BUFFER_LENGTH,
    redBuffer,
    &spo2Value,
    &spo2Valid,
    &algoHeartRate,
    &heartRateValid
  );

  if (spo2Value < 70 || spo2Value > 100) {
    spo2Valid = 0;
  }

  lastSpo2Time = millis();
}

// Print data on display if they are changed
void updateOutputIfNeeded(long irValue) {
  if (millis() - lastDisplayTime < DISPLAY_INTERVAL) {
    return;
  }

  printSensorData(irValue);
  updateDisplay(irValue);

  lastDisplayTime = millis();
}

// Function that prints BPM and SPO2 values in serial monitor
void printSensorData(long irValue) {
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
}

// Function that prints BPM and SPO2 vaules on OLED display
void updateDisplay(long irValue) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (irValue < FINGER_THRESHOLD) {
    display.setTextSize(1);
    display.setCursor(10, 8);
    display.print("Place finger on");
    display.setCursor(28, 20);
    display.print("sensor...");
  } 
  
  else if (!bufferFilled) {
    display.setTextSize(1);
    display.setCursor(0, 4);
    display.print("Calibrating BPM:");
    display.setCursor(0, 18);
    display.print(validReadings);
    display.print("/");
    display.print(RATE_SIZE);
    display.print(" readings");
  } 
  
  else {
    display.setTextSize(2);

    display.setCursor(0, 0);
    display.print("BPM:");
    display.print(beatAvg);

    display.setCursor(0, 17);
    display.print("SpO2:");

    if (spo2Valid) {
      display.print(spo2Value);
      display.print("%");
    } else {
      display.print("--");
    }
  }

  display.display();
}