#include <Wire.h> // for I2C communication troughout SDA/SCL
#include <Adafruit_LIS3DH.h> // library with  already built-in functions for Lis3DH
#include <Adafruit_Sensor.h> // general library used by Adafruit to manage sensors

Adafruit_LIS3DH  LISA= Adafruit_LIS3DH(); // to create an objective named LISA and it represent my sensor

const int int1Pin= 27; // creating a constant variable to define in which pin is INT1
unsigned long timestamp= 0; // unsined log is the type of variable that can contain int big values. and indicates how many MS passed 
//accension of esp32

void writeRegister(uint8_t reg, uint8_t value) { // we are creating a function that helps us set up the address and value of 
// each register that we will define in the set up
  Wire.beginTransmission(0x18);  // 0x18 is the sensor's I2C address
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}


void setup() {
  
  Serial.begin(115200); // opens the serial communication at certain baud rate
  pinMode(int1Pin, INPUT); // to tell the compiler the int1 pin is in input to send the values to ESP32

LISA.begin(0x18); // starts the I2C communication and 0x18 is the address to the sensor
LISA.setRange(LIS3DH_RANGE_2_G); // to set the max acceleration range that can be measured 
// the sensor calculates the movement on 3 axis (x,y,z). x measures the lateral movemnt (left-, right+), while y meausres 
// the frontal movement (forward+, backwards-), lastly z measures vertical movement/gravity (up+, down-)
// it detects gravity because ther is a small mass inside of the sensor. The range can go from 2 which is 
//more detailed and detectes small movements and goes till 16 which will be less detailed and detects bigger movements

writeRegister(0x32, 20); // used to write a value into a specific register inside the sensor. Register 
// are small memory locations inside the LIS3DH and each register has a specific address and a specific function
// in our case we will use the register with 0x32 bacuse it manages the thresholds. Regarding the value a small
//one makes the sensor more sensitive, while a bigger value makes it less sensitive
writeRegister(0x33, 10);// is used to set the duration of the trigger( acceleraation). how long the acceleration
// must remain above the threshold before the interrupt is generated
writeRegister(0x30, 0x2A);// is the register that configures which movement the INT1 needs to capture
//and 0x2A is the selected axis/direction
writeRegister(0x22, 0x40); // controls the interrupt pin behaviour and decides where it has to be sent

  Serial.println("Sensor ready");
}

void loop() {
int motion= digitalRead(int1Pin); // variable motion needed to salve temporarly the result, we use digitalRead
// because the piin INT1 sends digital signals (0 or 1)


if (motion == HIGH) {
  Serial.println("1");
}
else {
  Serial.println("0");
}

timestamp= millis();

Serial.print("Timestamp:");
Serial.println(timestamp);

delay(500);


}
