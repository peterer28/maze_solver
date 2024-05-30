#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <TCS3200.h>

#ifdef _ESP32_HAL_I2C_H_
#define SDA_PIN 21
#define SCL_PIN 22
#endif

MPU9250_asukiaaa mySensor;
TCS3200 colorSensor(15, 2, 27, 4, 16); // S0, S1, S2, S3, OUT pins for the color sensor

const int lineSensorFront = 34;
const int lineSensorBack = 35;
const int sensorFront = 32;
const int sensorLeft = 33;
const int sensorRight = 25;
const int sensorBack = 26;

void setup() {
  Serial.begin(115200);
  while (!Serial);

#ifdef _ESP32_HAL_I2C_H_ // For ESP32
  Wire.begin(SDA_PIN, SCL_PIN);
  mySensor.setWire(&Wire);
#endif

  mySensor.beginGyro();
  colorSensor.begin();
  colorSensor.frequency_scaling(TCS3200_OFREQ_20P); // Set color sensor frequency scaling to 20%

  pinMode(lineSensorFront, INPUT);
  pinMode(lineSensorBack, INPUT);
  pinMode(sensorFront, INPUT);
  pinMode(sensorLeft, INPUT);
  pinMode(sensorRight, INPUT);
  pinMode(sensorBack, INPUT);

  Serial.println("MPU9250 gyroscope and TCS3200 color sensor initialized");
}

void updateGyro() {
  int result = mySensor.gyroUpdate();
  if (result == 0) {
    float gyroX = mySensor.gyroX();
    float gyroY = mySensor.gyroY();
    float gyroZ = mySensor.gyroZ();

    Serial.print("Gyro X: ");
    Serial.print(gyroX);
    Serial.print(", Gyro Y: ");
    Serial.print(gyroY);
    Serial.print(", Gyro Z: ");
    Serial.println(gyroZ);
  } else {
    Serial.println("Cannot read gyro values, error code: " + String(result));
  }
}

void readLineSensors() {
  int frontLine = digitalRead(lineSensorFront);
  int backLine = digitalRead(lineSensorBack);

  Serial.print("Line Sensor Front: ");
  Serial.print(frontLine);
  Serial.print(", Line Sensor Back: ");
  Serial.println(backLine);
}

void readProximitySensors() {
  int front = !digitalRead(sensorFront);
  int left = !digitalRead(sensorLeft);
  int right = !digitalRead(sensorRight);
  int back = !digitalRead(sensorBack);

  Serial.print("Proximity Sensor Front: ");
  Serial.print(front);
  Serial.print(", Left: ");
  Serial.print(left);
  Serial.print(", Right: ");
  Serial.print(right);
  Serial.print(", Back: ");
  Serial.println(back);
}

void readColorSensor() {
  RGBColor rgb_color = colorSensor.read_rgb_color();

  Serial.print("Color Sensor - Red: ");
  Serial.print(rgb_color.red);
  Serial.print(" Green: ");
  Serial.print(rgb_color.green);
  Serial.print(" Blue: ");
  Serial.println(rgb_color.blue);
}

void loop() {
  Serial.println("Reading sensors...");

  updateGyro();
  readLineSensors();
  readProximitySensors();
  readColorSensor();

  Serial.println("------------------------------");
  delay(1000); // Wait before the next iteration
}
