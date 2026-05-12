#include <Wire.h>             // for I2C communication throughout SDA/SCL
#include <Adafruit_LIS3DH.h>  // library with already built-in functions for LIS3DH
#include <Adafruit_Sensor.h>  // general library used by Adafruit to manage sensors

Adafruit_LIS3DH LISA = Adafruit_LIS3DH(); // create an object named LISA representing the sensor

const int int1Pin = 26;    // constant variable to define on which pin INT1 is connected
unsigned long timestamp = 0;

// Helper function to read a single register from the sensor via I2C
uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(0x18);  // 0x18 is the sensor's I2C address
  Wire.write(reg);
  Wire.endTransmission(false);   // false = repeated start, keeps the bus active for reading
  Wire.requestFrom((uint8_t)0x18, (uint8_t)1);
  return Wire.read();
}

// Helper function to write a value into a specific register via I2C
void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(0x18);  // 0x18 is the sensor's I2C address
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void setup() {

  Serial.begin(115200);             // opens the serial communication at a certain baud rate
  pinMode(int1Pin, INPUT);          // INT1 pin is configured as input to receive signals from the sensor

  LISA.begin(0x18);                 // starts I2C communication; 0x18 is the sensor's I2C address
  LISA.setRange(LIS3DH_RANGE_2_G);  // sets the max acceleration range to 2G for maximum sensitivity
  // The sensor measures movement on 3 axes (X, Y, Z):
  //   X = lateral movement (left -, right +)
  //   Y = frontal movement (forward +, backward -)
  //   Z = vertical movement / gravity (up +, down -)
  // The 2G range provides the finest resolution (~1 mg per LSB in high-resolution mode),
  // which is ideal for detecting small movements on a breadboard.

  // FIX 1 — CTRL_REG1 (0x20): explicitly enable all three axes and set ODR to 100 Hz.
  // Without this, the sensor may remain in power-down mode and produce no samples.
  // 0x57 = 0101 0111 → ODR = 100 Hz, low-power off, Z/Y/X axes all enabled.
  writeRegister(0x20, 0x57);

  // FIX 2 — INT1_THS (0x32): threshold for interrupt generation.
  // In 2G range, each LSB = ~16 mg. A value of 4 corresponds to ~64 mg,
  // which is low enough to detect small shakes on a breadboard.
  // Original value was 10 (~160 mg), which may be too high for limited movement.
  writeRegister(0x32, 4);

  // FIX 3 — INT1_DURATION (0x33): how many consecutive samples must exceed the threshold
  // before the interrupt fires. At 100 Hz, 1 sample = 10 ms.
  // Setting this to 1 means the interrupt fires after a single sample above the threshold,
  // making detection immediate. Original value was 5 (= 50 ms sustained movement required).
  writeRegister(0x33, 1);

  // INT1_CFG (0x30): configure which axes and directions trigger the interrupt.
  // 0x2A = 0010 1010 → enables high-event detection on X, Y, Z (OR combination).
  // This means any single axis exceeding the threshold will trigger the interrupt.
  writeRegister(0x30, 0x2A);

  // CTRL_REG3 (0x22): route the IA1 interrupt signal to the physical INT1 pin.
  // 0x40 = bit 6 set → I1_IA1 enabled, INT1 pin goes HIGH when interrupt fires.
  writeRegister(0x22, 0x40);

  // FIX 4 — CTRL_REG5 (0x24): enable interrupt latching.
  // Without latching, INT1 goes HIGH for only a very brief moment when movement is detected
  // and then immediately drops back to LOW. Since the loop runs every 200 ms, a short pulse
  // would almost always be missed by digitalRead().
  // 0x08 = bit 3 set → LIR_INT1 = 1, interrupt on INT1 is latched (stays HIGH until cleared).
  writeRegister(0x24, 0x08);

  Serial.println("Sensor ready");
}

void loop() {

  // Read the INT1 pin: HIGH means movement was detected above the threshold
  int motion = digitalRead(int1Pin);

  if (motion == HIGH) {
    Serial.println("1");

    // FIX 5 — Clear the latched interrupt by reading the INT1_SRC register (0x31).
    // After latching is enabled, the INT1 pin stays HIGH until this register is read.
    // Reading it resets the interrupt flag and allows the next event to be detected.
    readRegister(0x31);
  }
  else {
    Serial.println("0");
  }

  timestamp = millis();
  Serial.print("Timestamp:");
  Serial.println(timestamp);

  delay(200);
}
