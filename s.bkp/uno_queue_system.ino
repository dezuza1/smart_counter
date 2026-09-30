#include <Servo.h>

// ================= PIN CONFIGURATION =================

const int ENTRY_SENSOR_PIN = 2;
const int EXIT_SENSOR_PIN  = 3;

const int SERVO_PIN = 9;
const int LED_PIN   = 7;


// ================= QUEUE SETTINGS =================

const int MAX_CAPACITY = 5;


// ================= SERVO SETTINGS =================

const int GATE_OPEN_ANGLE   = 90;
const int GATE_CLOSED_ANGLE = 0;


// ================= IR SENSOR SETTINGS =================

// Most IR obstacle sensors detect an object by giving LOW.
// If your sensor works opposite, change LOW to HIGH.
const int SENSOR_TRIGGERED = LOW;


// ================= VARIABLES =================

Servo gateServo;

int occupancy = 0;

// Used to remember previous sensor state
int previousEntryState = HIGH;
int previousExitState  = HIGH;


// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(9600);

  // IR sensors
  pinMode(ENTRY_SENSOR_PIN, INPUT);
  pinMode(EXIT_SENSOR_PIN, INPUT);

  // LED
  pinMode(LED_PIN, OUTPUT);

  // Servo
  gateServo.attach(SERVO_PIN);

  // Start with gate OPEN
  gateServo.write(GATE_OPEN_ANGLE);

  // LED ON because queue has space
  digitalWrite(LED_PIN, HIGH);

  Serial.println("----------------------------------");
  Serial.println(" SMART QUEUE MANAGEMENT SYSTEM");
  Serial.println("----------------------------------");

  Serial.println("Maximum Capacity: 5");
  Serial.println("Current Occupancy: 0");
  Serial.println("Gate: OPEN");
  Serial.println("LED: ON");
  Serial.println("----------------------------------");
}


// ======================================================
// OPEN GATE
// ======================================================

void openGate() {

  gateServo.write(GATE_OPEN_ANGLE);

  digitalWrite(LED_PIN, HIGH);

  Serial.println("GATE OPEN");
  Serial.println("SPACE AVAILABLE");
}


// ======================================================
// CLOSE GATE
// ======================================================

void closeGate() {

  gateServo.write(GATE_CLOSED_ANGLE);

  digitalWrite(LED_PIN, LOW);

  Serial.println("GATE CLOSED");
  Serial.println("QUEUE FULL");
}


// ======================================================
// CHECK ENTRY
// ======================================================

void checkEntry() {

  int currentEntryState = digitalRead(ENTRY_SENSOR_PIN);


  // Detect a new person entering
  // Sensor changes from NOT DETECTED → DETECTED

  if (currentEntryState == SENSOR_TRIGGERED &&
      previousEntryState != SENSOR_TRIGGERED) {

    // Only allow entry if space is available
    if (occupancy < MAX_CAPACITY) {

      occupancy++;

      Serial.println();
      Serial.println("PERSON ENTERED");

      Serial.print("Occupancy: ");
      Serial.print(occupancy);
      Serial.print(" / ");
      Serial.println(MAX_CAPACITY);


      // Check if maximum capacity reached

      if (occupancy >= MAX_CAPACITY) {

        closeGate();

      }
      else {

        openGate();

        Serial.print("Available spaces: ");
        Serial.println(MAX_CAPACITY - occupancy);
      }

    }
    else {

      Serial.println();
      Serial.println("ENTRY BLOCKED");
      Serial.println("QUEUE IS FULL");

    }
  }

  // Save current sensor state
  previousEntryState = currentEntryState;
}


// ======================================================
// CHECK EXIT
// ======================================================

void checkExit() {

  int currentExitState = digitalRead(EXIT_SENSOR_PIN);


  // Detect a new person exiting

  if (currentExitState == SENSOR_TRIGGERED &&
      previousExitState != SENSOR_TRIGGERED) {

    // Make sure occupancy doesn't become negative

    if (occupancy > 0) {

      occupancy--;

      Serial.println();
      Serial.println("PERSON EXITED");

      Serial.print("Occupancy: ");
      Serial.print(occupancy);
      Serial.print(" / ");
      Serial.println(MAX_CAPACITY);


      // Space is available again

      if (occupancy < MAX_CAPACITY) {

        openGate();

        Serial.print("Available spaces: ");
        Serial.println(MAX_CAPACITY - occupancy);
      }

    }
    else {

      Serial.println();
      Serial.println("QUEUE IS ALREADY EMPTY");

    }
  }

  // Save current sensor state
  previousExitState = currentExitState;
}


// ======================================================
// MAIN LOOP
// ======================================================

void loop() {

  checkEntry();

  checkExit();

  delay(50);
}