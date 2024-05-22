#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "your_ssid";
const char* password = "your_password";
const char* serverName = "http://your_server_ip:5000/update_maze";

const int motorLeftForward = 5;
const int motorLeftBackward = 18;
const int motorRightForward = 19;
const int motorRightBackward = 21;
const int sensorFront = 32;
const int sensorLeft = 33;
const int sensorRight = 25;
const int sensorBack = 26;

volatile int leftStepCount = 0;
volatile int rightStepCount = 0;
const int stepsPerRotation = 12;  // Number of teeth on the wheel
const int stepsPerGridCell = 100;  // Adjust this value as needed for one grid cell movement
const int pwmDutyCycle = 128;  // Adjust as needed for motor speed (0-255)

enum Direction { UP, DOWN, LEFT, RIGHT };
Direction currentDirection = UP;  // Starting direction

int currentPosition[2] = {6, 2};  // Starting position
const int goalPosition[2] = {4, 3};

// Interrupt Service Routine (ISR) for KLR-512 sensors
void IRAM_ATTR onLeftStep() {
    leftStepCount++;
}

void IRAM_ATTR onRightStep() {
    rightStepCount++;
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    pinMode(motorLeftForward, OUTPUT);
    pinMode(motorLeftBackward, OUTPUT);
    pinMode(motorRightForward, OUTPUT);
    pinMode(motorRightBackward, OUTPUT);
    pinMode(sensorFront, INPUT);
    pinMode(sensorLeft, INPUT);
    pinMode(sensorRight, INPUT);
    pinMode(sensorBack, INPUT);

    attachInterrupt(digitalPinToInterrupt(sensorLeft), onLeftStep, RISING);
    attachInterrupt(digitalPinToInterrupt(sensorRight), onRightStep, RISING);
}

void moveForward() {
    leftStepCount = 0;
    rightStepCount = 0;
    while (leftStepCount < stepsPerGridCell && rightStepCount < stepsPerGridCell) {
        analogWrite(motorLeftForward, pwmDutyCycle);
        analogWrite(motorRightForward, pwmDutyCycle);
    }
    analogWrite(motorLeftForward, 0);
    analogWrite(motorRightForward, 0);
    updatePosition();
}

void moveBackward() {
    leftStepCount = 0;
    rightStepCount = 0;
    while (leftStepCount < stepsPerGridCell && rightStepCount < stepsPerGridCell) {
        analogWrite(motorLeftBackward, pwmDutyCycle);
        analogWrite(motorRightBackward, pwmDutyCycle);
    }
    analogWrite(motorLeftBackward, 0);
    analogWrite(motorRightBackward, 0);
    updatePosition(false);
}

void turnLeft() {
    leftStepCount = 0;
    rightStepCount = 0;
    while (leftStepCount < stepsPerRotation / 2 && rightStepCount < stepsPerRotation / 2) {
        analogWrite(motorLeftBackward, pwmDutyCycle);
        analogWrite(motorRightForward, pwmDutyCycle);
    }
    analogWrite(motorLeftBackward, 0);
    analogWrite(motorRightForward, 0);
    updateDirection(false);
}

void turnRight() {
    leftStepCount = 0;
    rightStepCount = 0;
    while (leftStepCount < stepsPerRotation / 2 && rightStepCount < stepsPerRotation / 2) {
        analogWrite(motorLeftForward, pwmDutyCycle);
        analogWrite(motorRightBackward, pwmDutyCycle);
    }
    analogWrite(motorLeftForward, 0);
    analogWrite(motorRightBackward, 0);
    updateDirection(true);
}

void updatePosition(bool forward = true) {
    if (forward) {
        switch (currentDirection) {
            case UP:    currentPosition[0]--; break;
            case DOWN:  currentPosition[0]++; break;
            case LEFT:  currentPosition[1]--; break;
            case RIGHT: currentPosition[1]++; break;
        }
    } else {
        switch (currentDirection) {
            case UP:    currentPosition[0]++; break;
            case DOWN:  currentPosition[0]--; break;
            case LEFT:  currentPosition[1]++; break;
            case RIGHT: currentPosition[1]--; break;
        }
    }
}

void updateDirection(bool turnRight) {
    if (turnRight) {
        switch (currentDirection) {
            case UP:    currentDirection = RIGHT; break;
            case RIGHT: currentDirection = DOWN; break;
            case DOWN:  currentDirection = LEFT; break;
            case LEFT:  currentDirection = UP; break;
        }
    } else {
        switch (currentDirection) {
            case UP:    currentDirection = LEFT; break;
            case LEFT:  currentDirection = DOWN; break;
            case DOWN:  currentDirection = RIGHT; break;
            case RIGHT: currentDirection = UP; break;
        }
    }
}

void sendMazeToServer(String mazeJson) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverName);
        http.addHeader("Content-Type", "application/json");
        int httpResponseCode = http.POST(mazeJson);
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println(response);
            // Process server response to get the next move
            StaticJsonDocument<500> doc;
            deserializeJson(doc, response);
            const char* instruction = doc["instructions"][0];
            if (instruction) {
                executeMove(instruction);
            }
        } else {
            Serial.println("Error in HTTP request");
        }
        http.end();
    } else {
        Serial.println("Disconnected from WiFi");
    }
}

void executeMove(const char* instruction) {
    if (strcmp(instruction, "MOVE_UP") == 0) {
        moveForward();
    } else if (strcmp(instruction, "MOVE_DOWN") == 0) {
        moveBackward();
    } else if (strcmp(instruction, "MOVE_LEFT") == 0) {
        turnLeft();
        moveForward();
    } else if (strcmp(instruction, "MOVE_RIGHT") == 0) {
        turnRight();
        moveForward();
    }
}

void loop() {
    // Read sensors
    int front = digitalRead(sensorFront);
    int left = digitalRead(sensorLeft);
    int right = digitalRead(sensorRight);
    int back = digitalRead(sensorBack);

    // Update the maze with sensor data
    String mazeJson = "{\"matrix\": [";
    mazeJson += "[\"*\",\"*\", \"*\", \"*\", \"*\", \"*\", \"*\", \"*\"],";
    mazeJson += "[\"*\",\" \", \" \", \" \", \" \", \" \", \" \", \"*\"],";
    mazeJson += "[\"*\",\" \", \" \", \" \", \" \", \" \", \" \", \"*\"],";
    mazeJson += "[\"*\",\" \", \" \", \" \", \" \", \" \", \" \", \"*\"],";
    mazeJson += "[\"*\",\" \", \" \", \"p\", \" \", \" \", \" \", \"*\"],";
    mazeJson += "[\"*\",\" \", \" \", \" \", \" \", \" \", \" \", \"*\"],";
    mazeJson += "[\"*\",\"g\", ";
    mazeJson += left ? "\"*\"" : "\" \"";
    mazeJson += ", ";
    mazeJson += front ? "\"*\"" : "\" \"";
    mazeJson += ", ";
    mazeJson += right ? "\"*\"" : "\" \"";
    mazeJson += ", \" \", \"*\"],";
    mazeJson += "[\"*\",\"*\", \"*\", \"*\", \"*\", \"*\", \"*\", \"*\"]";
    mazeJson += "], \"current\": [" + String(currentPosition[0]) + ", " + String(currentPosition[1]) + "], \"goal\": [4, 3]}";

    // Send updated maze to server
    sendMazeToServer(mazeJson);

    // Wait before the next iteration
    delay(1000);
}
