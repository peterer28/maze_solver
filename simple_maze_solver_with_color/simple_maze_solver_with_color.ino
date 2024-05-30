#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <TCS3200.h>

#ifdef _ESP32_HAL_I2C_H_
#define SDA_PIN 21
#define SCL_PIN 22
#endif

MPU9250_asukiaaa mySensor;
TCS3200 colorSensor(15, 2, 27, 4, 16); // S0, S1, S2, S3, OUT pins for the color sensor

const int motorLeftForward = 5;
const int motorLeftBackward = 18;
const int motorRightForward = 19;
const int motorRightBackward = 23;
const int lineSensorFront = 34;
const int lineSensorBack = 35;
const int sensorFront = 32;
const int sensorLeft = 33;
const int sensorRight = 25;
const int sensorBack = 26;

// PWM channels
const int pwmChannelLeftForward = 0;
const int pwmChannelLeftBackward = 1;
const int pwmChannelRightForward = 2;
const int pwmChannelRightBackward = 3;

// PWM settings
const int pwmFrequency = 5000;
const int pwmResolution = 8;
int pwmDutyCycle = 128; // Adjust as needed (0-255)

// Variables for gyroscope
float angleZ = 0;
unsigned long previousTime = 0;
float elapsedTime = 0;

const int stepDegrees = 70; // Updated based on your findings
const int numSteps = 1;
const int stepDelay = 200; // 200 milliseconds

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

  // Set up PWM channels
  ledcSetup(pwmChannelLeftForward, pwmFrequency, pwmResolution);
  ledcSetup(pwmChannelLeftBackward, pwmFrequency, pwmResolution);
  ledcSetup(pwmChannelRightForward, pwmFrequency, pwmResolution);
  ledcSetup(pwmChannelRightBackward, pwmFrequency, pwmResolution);

  // Attach PWM channels to motor pins
  ledcAttachPin(motorLeftForward, pwmChannelLeftForward);
  ledcAttachPin(motorLeftBackward, pwmChannelLeftBackward);
  ledcAttachPin(motorRightForward, pwmChannelRightForward);
  ledcAttachPin(motorRightBackward, pwmChannelRightBackward);

  pinMode(lineSensorFront, INPUT);
  pinMode(lineSensorBack, INPUT);
  pinMode(sensorFront, INPUT);
  pinMode(sensorLeft, INPUT);
  pinMode(sensorRight, INPUT);
  pinMode(sensorBack, INPUT);

  Serial.println("MPU9250 gyroscope and TCS3200 color sensor initialized");

  previousTime = millis();
}

void updateGyro() {
  int result = mySensor.gyroUpdate();
  if (result == 0) {
    float gyroZ = mySensor.gyroZ();

    unsigned long currentTime = millis();
    elapsedTime = (currentTime - previousTime) / 1000.0; // Convert to seconds
    previousTime = currentTime;

    angleZ += gyroZ * elapsedTime; // Update angle based on gyro reading

    // Print the gyroscope Z value and the current angle to the serial monitor
    Serial.print("Gyro Z: ");
    Serial.print(gyroZ);
    Serial.print(", Angle Z: ");
    Serial.println(angleZ);
  } else {
    Serial.println("Cannot read gyro values, error code: " + String(result));
  }
}

void moveForward() {
  // Reset line sensors
  bool frontLineDetected = false;
  bool backLineDetected = false;

  // Start moving forward
  ledcWrite(pwmChannelLeftForward, pwmDutyCycle);
  ledcWrite(pwmChannelRightForward, pwmDutyCycle);

  // Wait until both line sensors detect the lines sequentially
  while (!frontLineDetected || !backLineDetected) {
    if (!frontLineDetected && digitalRead(lineSensorFront) == HIGH) {
      frontLineDetected = true;
    }
    if (frontLineDetected && !backLineDetected && digitalRead(lineSensorBack) == HIGH) {
      backLineDetected = true;
    }
  }

  // Stop motors
  ledcWrite(pwmChannelLeftForward, 0);
  ledcWrite(pwmChannelRightForward, 0);
}

void moveBackward() {
  // Reset line sensors
  bool frontLineDetected = false;
  bool backLineDetected = false;

  // Start moving backward
  ledcWrite(pwmChannelLeftBackward, pwmDutyCycle);
  ledcWrite(pwmChannelRightBackward, pwmDutyCycle);

  // Wait until both line sensors detect the lines sequentially
  while (!frontLineDetected || !backLineDetected) {
    if (!frontLineDetected && digitalRead(lineSensorBack) == HIGH) {
      frontLineDetected = true;
    }
    if (frontLineDetected && !backLineDetected && digitalRead(lineSensorFront) == HIGH) {
      backLineDetected = true;
    }
  }

  // Stop motors
  ledcWrite(pwmChannelLeftBackward, 0);
  ledcWrite(pwmChannelRightBackward, 0);
}

void turnLeft() {
  angleZ = 0; // Reset angle
  delay(1000); // Wait before starting the turn

  for (int i = 0; i < numSteps; i++) {
    // Start turning left
    ledcWrite(pwmChannelLeftBackward, pwmDutyCycle);
    ledcWrite(pwmChannelRightForward, pwmDutyCycle);

    // Turn by stepDegrees and then pause
    while (abs(angleZ) < (i + 1) * stepDegrees) {
      updateGyro();
      delay(10);
    }
    ledcWrite(pwmChannelLeftBackward, 0);
    ledcWrite(pwmChannelRightForward, 0);
    delay(stepDelay); // Pause between steps
  }
}

void turnRight() {
  angleZ = 0; // Reset angle
  delay(1000); // Wait before starting the turn

  for (int i = 0; i < numSteps; i++) {
    // Start turning right
    ledcWrite(pwmChannelLeftForward, pwmDutyCycle);
    ledcWrite(pwmChannelRightBackward, pwmDutyCycle);

    // Turn by stepDegrees and then pause
    while (abs(angleZ) < (i + 1) * stepDegrees) {
      updateGyro();
      delay(10);
    }
    ledcWrite(pwmChannelLeftForward, 0);
    ledcWrite(pwmChannelRightBackward, 0);
    delay(stepDelay); // Pause between steps
  }
}

void detectColor() {
  RGBColor rgb_color = colorSensor.read_rgb_color();

  Serial.print("Red: ");
  Serial.print(rgb_color.red);
  Serial.print(" Green: ");
  Serial.print(rgb_color.green);
  Serial.print(" Blue: ");
  Serial.println(rgb_color.blue);

  // Check if the detected color is red
  if (rgb_color.red > 200 && rgb_color.green < 100 && rgb_color.blue < 100) {
    Serial.println("Target found!");
    while (true) {
      // Stop the robot
      delay(1000);
    }
  }
}

void loop() {
  delay(1000); // Initial delay before starting the search

  // Read sensors
  int front = !digitalRead(sensorFront);// Inverting the reading
  int left = !digitalRead(sensorLeft);// Inverting the reading
  int right = !digitalRead(sensorRight);// Inverting the reading
  int back = !digitalRead(sensorBack);// Inverting the reading

// Determine the next move based on sensor readings using the provided table
  if (front == 0 && left == 0 && right == 0 && back == 0) {
    moveForward();
  } else if (front == 0 && left == 0 && right == 0 && back == 1) {
    moveForward();
  } else if (front == 0 && left == 0 && right == 1 && back == 0) {
    moveForward();
  } else if (front == 0 && left == 0 && right == 1 && back == 1) {
    moveForward();
  } else if (front == 0 && left == 1 && right == 0 && back == 0) {
    moveForward();
  } else if (front == 0 && left == 1 && right == 0 && back == 1) {
    moveForward();
  } else if (front == 0 && left == 1 && right == 1 && back == 0) {
    moveForward();
  } else if (front == 0 && left == 1 && right == 1 && back == 1) {
    moveForward();
  } else if (front == 1 && left == 0 && right == 0 && back == 0) {
    turnLeft();
    moveForward();
  } else if (front == 1 && left == 0 && right == 0 && back == 1) {
    turnLeft();
    moveForward();
  } else if (front == 1 && left == 0 && right == 1 && back == 0) {
    turnLeft();
    moveForward();
  } else if (front == 1 && left == 0 && right == 1 && back == 1) {
    turnLeft();
    moveForward();
  } else if (front == 1 && left == 1 && right == 0 && back == 0) {
    turnRight();
    moveForward();
  } else if (front == 1 && left == 1 && right == 0 && back == 1) {
    turnRight();
    moveForward();
  } else if (front == 1 && left == 1 && right == 1 && back == 0) {
    moveBackward();
  } else if (front == 1 && left == 1 && right == 1 && back == 1) {
    // No movement
    Serial.println("No movement possible.");
  }

  // Detect the color to check if the target is reached
  detectColor();

  delay(1000); // Wait before the next iteration
}

