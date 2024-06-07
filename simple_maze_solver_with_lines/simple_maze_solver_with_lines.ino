#include <Wire.h>
#include <MPU9250_asukiaaa.h>

#ifdef _ESP32_HAL_I2C_H_
#define SDA_PIN 21
#define SCL_PIN 22
#endif

MPU9250_asukiaaa mySensor;

const int motorLeftForward = 5;
const int motorLeftBackward = 18;
const int motorRightForward = 19;
const int motorRightBackward = 23;
const int sensorFront = 32;
const int sensorLeft = 33;
const int sensorRight = 25;
const int sensorCenterLeft = 2;
const int sensorCenterRight = 4;
const int sensorBack = 26;
const int lineSensorFront = 34;
const int lineSensorBack = 35;

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

  pinMode(sensorFront, INPUT);
  pinMode(sensorLeft, INPUT);
  pinMode(sensorRight, INPUT);
  pinMode(sensorCenterLeft, INPUT);
  pinMode(sensorCenterRight, INPUT);
  pinMode(sensorBack, INPUT);
  pinMode(lineSensorFront, INPUT);
  pinMode(lineSensorBack, INPUT);

  Serial.println("MPU9250 gyroscope initialized");

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
  Serial.println("Moving forward...");
  // Start moving forward with self-centering
  bool frontLineDetected = false;
  bool backLineDetected = false;

  while ((!frontLineDetected || !backLineDetected) && (!digitalRead(sensorFront) == LOW)) {
    // Read proximity sensors
    int left = !digitalRead(sensorCenterLeft);
    int right = !digitalRead(sensorCenterRight);

    // Self-centering logic
    if (left == LOW && right == LOW) {
      // doesn't detect wall on either side
      ledcWrite(pwmChannelLeftForward, pwmDutyCycle);
      ledcWrite(pwmChannelRightForward, pwmDutyCycle);
      Serial.println("No wall detected, moving forward");

    } else if (left == HIGH && right == LOW) {
      // Only right sensor detects wall, correct to the left
      ledcWrite(pwmChannelRightForward, 0);
      ledcWrite(pwmChannelLeftForward, pwmDutyCycle);
      Serial.println("Correcting to the right");

    } else if (left == LOW && right == HIGH) {
      // Only left sensor detects wall, correct to the right
      ledcWrite(pwmChannelRightForward, pwmDutyCycle);
      ledcWrite(pwmChannelLeftForward, 0);
      Serial.println("Correcting to the left");

    } else {
      // Both sensors detect wall, move forward
      ledcWrite(pwmChannelLeftForward, pwmDutyCycle);
      ledcWrite(pwmChannelRightForward, pwmDutyCycle);
      Serial.println("Wall detected on both sides, moving forward");
    }

    // Check if the target is reached
    if (digitalRead(lineSensorFront) == HIGH && digitalRead(lineSensorBack) == HIGH) {
      Serial.println("Target found!");
      while (true) {
        // Stop the robot
        ledcWrite(pwmChannelLeftForward, 0);
        ledcWrite(pwmChannelRightForward, 0);
        delay(1000);
      }
    }

    // Stop moving if a wall is detected in front
    if (!digitalRead(sensorFront) == HIGH) {
      Serial.println("Wall detected in front, stopping");
      break;
    }

    // Update line sensor status
    if (digitalRead(lineSensorFront) == HIGH) {
      frontLineDetected = true;
    }
    if (digitalRead(lineSensorBack) == HIGH) {
      backLineDetected = true;
    }
  }

  // Stop motors when a wall is detected in front
  ledcWrite(pwmChannelLeftForward, 0);
  ledcWrite(pwmChannelRightForward, 0);
  Serial.println("Stopping motors");
}

void moveBackward() {
  Serial.println("Moving backward...");
  // Start moving backward with self-centering
  bool frontLineDetected = false;
  bool backLineDetected = false;

  while ((!frontLineDetected || !backLineDetected)&&(!digitalRead(sensorBack) == LOW)) {
    // Read proximity sensors
    int left = !digitalRead(sensorLeft);
    int right = !digitalRead(sensorRight);

    // Self-centering logic
    if (left == LOW && right == LOW) {
      // Doesn't detect wall on either side
      ledcWrite(pwmChannelLeftBackward, pwmDutyCycle);
      ledcWrite(pwmChannelRightBackward, pwmDutyCycle);
      Serial.println("No wall detected, moving backward");
    } else if (left == LOW && right == HIGH) {
      // Only right sensor detects wall, correct to the left
      ledcWrite(pwmChannelLeftBackward, pwmDutyCycle);
      ledcWrite(pwmChannelRightBackward, 0);
      Serial.println("Correcting to the left");
    } else if (left == HIGH && right == LOW) {
      // Only left sensor detects wall, correct to the right
      ledcWrite(pwmChannelLeftBackward, 0);
      ledcWrite(pwmChannelRightBackward, pwmDutyCycle);
      Serial.println("Correcting to the right");
    } else {
      // Both sensors detect wall, move backward
      ledcWrite(pwmChannelLeftBackward, pwmDutyCycle);
      ledcWrite(pwmChannelRightBackward, pwmDutyCycle);
      Serial.println("Wall detected on both sides, moving backward");
    }

    // Check if the target is reached
    if (digitalRead(lineSensorFront) == HIGH && digitalRead(lineSensorBack) == HIGH) {
      Serial.println("Target found!");
      while (true) {
        // Stop the robot
        ledcWrite(pwmChannelLeftBackward, 0);
        ledcWrite(pwmChannelRightBackward, 0);
        delay(1000);
      }
    }

    // Stop moving if a wall is detected at the back
    if (!digitalRead(sensorBack) == HIGH) {
      Serial.println("Wall detected at the back, stopping");
      break;
    }

    // Update line sensor status
    if (digitalRead(lineSensorFront) == HIGH) {
      frontLineDetected = true;
    }
    if (digitalRead(lineSensorBack) == HIGH) {
      backLineDetected = true;
    }
  }

  // Stop motors when a wall is detected at the back
  ledcWrite(pwmChannelLeftBackward, 0);
  ledcWrite(pwmChannelRightBackward, 0);
  Serial.println("Stopping motors");
}

void turnLeft() {
  Serial.println("Turning left...");
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
  Serial.println("Turning right...");
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

void checkTarget() {
  if (digitalRead(lineSensorFront) == HIGH && digitalRead(lineSensorBack) == HIGH) {
    Serial.println("Target found!");
    while (true) {
      // Stop the robot
      ledcWrite(pwmChannelLeftForward, 0);
      ledcWrite(pwmChannelRightForward, 0);
      delay(1000);
    }
  }
}

void loop() {
  delay(1000); // Initial delay before starting the search

  // Read sensors
  int front = !digitalRead(sensorFront); // Inverting the reading
  int left = !digitalRead(sensorLeft); // Inverting the reading
  int right = !digitalRead(sensorRight); // Inverting the reading
  int back = !digitalRead(sensorBack); // Inverting the reading

  Serial.print("Front sensor: ");
  Serial.print(front);
  Serial.print(", Left sensor: ");
  Serial.print(left);
  Serial.print(", Right sensor: ");
  Serial.print(right);
  Serial.print(", Back sensor: ");
  Serial.println(back);

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

  // Check if the target is reached after each movement
  checkTarget();

  delay(500); // Wait before the next iteration
}


