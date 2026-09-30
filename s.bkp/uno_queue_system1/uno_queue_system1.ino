#include <Servo.h>

// ---------- PINS ----------

const byte ENTRY_SENSOR = 2;
const byte EXIT_SENSOR  = 3;
const byte SERVO_PIN    = 9;
const byte LED_PIN      = 7;

// ---------- QUEUE ----------

const int MAX_CAPACITY = 5;
int occupancy = 0;

// ---------- SERVO ----------

// Change these if your gate needs different positions
const int GATE_OPEN = 180;
const int GATE_CLOSED = 0;

Servo gate;

// ---------- SENSOR ----------

// Most IR obstacle sensors are LOW when detecting an object
const byte SENSOR_DETECTED = LOW;

// Remember previous sensor states
byte lastEntryState = HIGH;
byte lastExitState = HIGH;


// =================================================
// SETUP
// =================================================

void setup() {

  Serial.begin(9600);

  pinMode(ENTRY_SENSOR, INPUT);
  pinMode(EXIT_SENSOR, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // Attach servo to D9
  gate.attach(SERVO_PIN);

  // Start with gate OPEN
  gate.write(GATE_OPEN);

  // Space available
  digitalWrite(LED_PIN, HIGH);

  Serial.println("================================");
  Serial.println(" SMART QUEUE MANAGEMENT SYSTEM");
  Serial.println("================================");
  Serial.println("Maximum Capacity: 5");
  Serial.println("Occupancy: 0 / 5");
  Serial.println("Gate: OPEN");
  Serial.println("LED: ON");
}


// =================================================
// OPEN GATE
// =================================================

void openGate() {

  gate.write(GATE_OPEN);

  digitalWrite(LED_PIN, HIGH);

  Serial.println("Gate OPEN");
}


// =================================================
// CLOSE GATE
// =================================================

void closeGate() {

  gate.write(GATE_CLOSED);

  digitalWrite(LED_PIN, LOW);

  Serial.println("Gate CLOSED");
}


// =================================================
// ENTRY SENSOR
// =================================================

void checkEntry() {

  byte currentState = digitalRead(ENTRY_SENSOR);

  // New object/person detected
  if (currentState == SENSOR_DETECTED &&
      lastEntryState != SENSOR_DETECTED) {

    if (occupancy < MAX_CAPACITY) {

      occupancy++;

      Serial.print("ENTRY -> Occupancy: ");
      Serial.print(occupancy);
      Serial.println(" / 5");

      // If capacity reached
      if (occupancy == MAX_CAPACITY) {

        closeGate();

        Serial.println("QUEUE FULL - ENTRY BLOCKED");
      }
      else {

        openGate();

        Serial.print("Spaces available: ");
        Serial.println(MAX_CAPACITY - occupancy);
      }
    }
    else {

      Serial.println("ENTRY BLOCKED - QUEUE FULL");
    }
  }

  lastEntryState = currentState;
}


// =================================================
// EXIT SENSOR
// =================================================

void checkExit() {

  byte currentState = digitalRead(EXIT_SENSOR);

  // New object/person detected
  if (currentState == SENSOR_DETECTED &&
      lastExitState != SENSOR_DETECTED) {

    if (occupancy > 0) {

      occupancy--;

      Serial.print("EXIT -> Occupancy: ");
      Serial.print(occupancy);
      Serial.println(" / 5");

      // Space is now available
      openGate();

      Serial.print("Spaces available: ");
      Serial.println(MAX_CAPACITY - occupancy);
    }
    else {

      Serial.println("QUEUE EMPTY");
    }
  }

  lastExitState = currentState;
}


// =================================================
// MAIN LOOP
// =================================================

void loop() {

  checkEntry();
  checkExit();

  delay(50);
}