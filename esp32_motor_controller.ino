#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "TAKTUS2.4G";
const char* password = "subtletech";

// A: right motor, B: left motor
#define RIGHT_MOTOR_IN1 16
#define RIGHT_MOTOR_IN2 17
#define LEFT_MOTOR_IN3  5
#define LEFT_MOTOR_IN4  18
#define RIGHT_MOTOR_ENA 4
#define LEFT_MOTOR_ENB  19

const int ROTATE_DURATION = 400;  // ~10° turn duration
const int ROTATE_PAUSE = 200;

WebServer server(80);

// ===== MOTOR FUNCTIONS =====
void moveForward() {
  Serial.println("moving forward");
  digitalWrite(RIGHT_MOTOR_IN1, HIGH);
  digitalWrite(RIGHT_MOTOR_IN2, LOW);
  digitalWrite(LEFT_MOTOR_IN3, LOW);
  digitalWrite(LEFT_MOTOR_IN4, HIGH);
}

void moveBackward() {
  Serial.println("moving backward");
  digitalWrite(RIGHT_MOTOR_IN1, LOW);
  digitalWrite(RIGHT_MOTOR_IN2, HIGH);
  digitalWrite(LEFT_MOTOR_IN3, HIGH);
  digitalWrite(LEFT_MOTOR_IN4, LOW);
}

void turnLeft() {
  Serial.println("turning left");
  digitalWrite(RIGHT_MOTOR_IN1, LOW);
  digitalWrite(RIGHT_MOTOR_IN2, HIGH);
  digitalWrite(LEFT_MOTOR_IN3, LOW);
  digitalWrite(LEFT_MOTOR_IN4, HIGH);
  delay(ROTATE_DURATION);
  stopMotors();
  delay(ROTATE_PAUSE);
}

void turnRight() {
  Serial.println("turning right");
  digitalWrite(RIGHT_MOTOR_IN1, HIGH);
  digitalWrite(RIGHT_MOTOR_IN2, LOW);
  digitalWrite(LEFT_MOTOR_IN3, HIGH);
  digitalWrite(LEFT_MOTOR_IN4, LOW);
  delay(ROTATE_DURATION);
  stopMotors();
  delay(ROTATE_PAUSE);
}

void stopMotors() {
  Serial.println("stopping motors");
  digitalWrite(RIGHT_MOTOR_IN1, LOW);
  digitalWrite(RIGHT_MOTOR_IN2, LOW);
  digitalWrite(LEFT_MOTOR_IN3, LOW);
  digitalWrite(LEFT_MOTOR_IN4, LOW);
}

// ===== HTTP HANDLERS =====
void handleRoot() {
  server.send(200, "text/plain", "ESP32 Motor Controller is Online");
}

void handleForward() {
  moveForward();
  server.send(200, "text/plain", "Moving forward");
}

void handleBackward() {
  moveBackward();
  server.send(200, "text/plain", "Moving backward");
}

void handleLeft() {
  turnLeft();
  server.send(200, "text/plain", "Turning left");
}

void handleRight() {
  turnRight();
  server.send(200, "text/plain", "Turning right");
}

void handleStop() {
  stopMotors();
  server.send(200, "text/plain", "Stopping motors");
}

void handleNotFound() {
  server.send(404, "text/plain", "Command not found");
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  // Motor pins
  pinMode(RIGHT_MOTOR_ENA, OUTPUT);
  pinMode(RIGHT_MOTOR_IN1, OUTPUT);
  pinMode(RIGHT_MOTOR_IN2, OUTPUT);
  pinMode(LEFT_MOTOR_IN3, OUTPUT);
  pinMode(LEFT_MOTOR_IN4, OUTPUT);
  pinMode(LEFT_MOTOR_ENB, OUTPUT);

  digitalWrite(RIGHT_MOTOR_IN1, LOW);
  digitalWrite(RIGHT_MOTOR_IN2, LOW);
  digitalWrite(LEFT_MOTOR_IN3, LOW);
  digitalWrite(LEFT_MOTOR_IN4, LOW);

  analogWrite(RIGHT_MOTOR_ENA, 200);
  analogWrite(LEFT_MOTOR_ENB, 200);

  // Wi-Fi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Setup routes
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

// ===== LOOP =====
void loop() {
  server.handleClient();

  // Optional: Accept serial commands too
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    Serial.println("Received: " + command);

    if (command == "0.50,0.00" || command == "forward") moveForward();
    else if (command == "-0.50,0.00" || command == "backward") moveBackward();
    else if (command == "0.00,0.50" || command == "right") turnRight();
    else if (command == "0.00,-0.50" || command == "left") turnLeft();
    else if (command == "0.00,0.00" || command == "stop") stopMotors();
    else Serial.println("Unknown command.");
  }
}

