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
const int encoderLeft = 34;
const int encoderRight = 35;
const int sensorFront = 32;
const int sensorLeft = 33;
const int sensorRight = 25;
const int sensorBack = 26;

volatile int leftEncoderCount = 0;
volatile int rightEncoderCount = 0;
int moveSteps = 100; // Adjust as needed for one grid cell
int pwmDutyCycle = 128; // Adjust as needed for motor speed (0-255)

void IRAM_ATTR onLeftEncoder() {
    leftEncoderCount++;
}

void IRAM_ATTR onRightEncoder() {
    rightEncoderCount++;
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
    pinMode(encoderLeft, INPUT_PULLUP);
    pinMode(encoderRight, INPUT_PULLUP);
    pinMode(sensorFront, INPUT);
    pinMode(sensorLeft, INPUT);
    pinMode(sensorRight, INPUT);
    pinMode(sensorBack, INPUT);

    attachInterrupt(digitalPinToInterrupt(encoderLeft), onLeftEncoder, RISING);
    attachInterrupt(digitalPinToInterrupt(encoderRight), onRightEncoder, RISING);
}

void moveForward() {
    leftEncoderCount = 0;
    rightEncoderCount = 0;
    while (leftEncoderCount < moveSteps && rightEncoderCount < moveSteps) {
        analogWrite(motorLeftForward, pwmDutyCycle);
        analogWrite(motorRightForward, pwmDutyCycle);
    }
    analogWrite(motorLeftForward, 0);
    analogWrite(motorRightForward, 0);
}

void moveBackward() {
    leftEncoderCount = 0;
    rightEncoderCount = 0;
    while (leftEncoderCount < moveSteps && rightEncoderCount < moveSteps) {
        analogWrite(motorLeftBackward, pwmDutyCycle);
        analogWrite(motorRightBackward, pwmDutyCycle);
    }
    analogWrite(motorLeftBackward, 0);
    analogWrite(motorRightBackward, 0);
}

void turnLeft() {
    leftEncoderCount = 0;
    rightEncoderCount = 0;
    while (leftEncoderCount < moveSteps / 2 && rightEncoderCount < moveSteps / 2) {
        analogWrite(motorLeftBackward, pwmDutyCycle);
        analogWrite(motorRightForward, pwmDutyCycle);
    }
    analogWrite(motorLeftBackward, 0);
    analogWrite(motorRightForward, 0);
}

void turnRight() {
    leftEncoderCount = 0;
    rightEncoderCount = 0;
    while (leftEncoderCount < moveSteps / 2 && rightEncoderCount < moveSteps / 2) {
        analogWrite(motorLeftForward, pwmDutyCycle);
        analogWrite(motorRightBackward, pwmDutyCycle);
    }
    analogWrite(motorLeftForward, 0);
    analogWrite(motorRightBackward, 0);
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
    mazeJson += "[\" \", \" \", \" \", \" \", \" \"],";
    mazeJson += "[\" \", \"*\", \"*\", \"*\", \" \"],";
    mazeJson += "[\" \", \"*\", \"p\", \"*\", \" \"],";
    mazeJson += "[\" \", \"*\", \"*\", \"*\", \" \"],";
    mazeJson += "[\"g\", ";
    mazeJson += left ? "\"*\"" : "\" \"";
    mazeJson += ", ";
    mazeJson += front ? "\"*\"" : "\" \"";
    mazeJson += ", ";
    mazeJson += right ? "\"*\"" : "\" \"";
    mazeJson += ", \" \", \" \"]";
    mazeJson += "], \"start\": [4, 0], \"goal\": [2, 2]}";

    // Send updated maze to server
    sendMazeToServer(mazeJson);

    // Wait before the next iteration
    delay(1000);
}


