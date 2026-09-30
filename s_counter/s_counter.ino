// =================================================
// SMART QUEUE MANAGEMENT SYSTEM
// =================================================
// Hardware:
// Arduino Uno
// 2x IR Sensors
// 1x Existing LED
// 1x New FULL LED
// 1x Buzzer
//
// NO SERVO MOTOR
// =================================================


// ---------- PINS ----------

const byte ENTRY_SENSOR = 2;
const byte EXIT_SENSOR  = 3;

const byte BUZZER_PIN   = 6;

// Existing LED - DO NOT CHANGE
const byte LED_PIN      = 7;

// New LED - ON only when queue is FULL
const byte FULL_LED_PIN = 8;


// ---------- QUEUE ----------

const int MAX_CAPACITY = 5;

int occupancy = 0;


// ---------- SENSOR ----------

// Most IR obstacle sensors output LOW
// when an object is detected

const byte SENSOR_DETECTED = LOW;


// ---------- SENSOR STATES ----------

byte lastEntryState = HIGH;
byte lastExitState  = HIGH;


// =================================================
// SETUP
// =================================================

void setup() {

  Serial.begin(9600);

  // IR sensors
  pinMode(ENTRY_SENSOR, INPUT);
  pinMode(EXIT_SENSOR, INPUT);

  // Existing LED
  pinMode(LED_PIN, OUTPUT);

  // New FULL LED
  pinMode(FULL_LED_PIN, OUTPUT);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);


  // Initial states

  digitalWrite(LED_PIN, HIGH);

  digitalWrite(FULL_LED_PIN, LOW);

  digitalWrite(BUZZER_PIN, LOW);


  // Startup messages

  Serial.println("================================");
  Serial.println(" SMART QUEUE MANAGEMENT SYSTEM");
  Serial.println("================================");

  Serial.println("Maximum Capacity: 5");

  Serial.println("Occupancy: 0 / 5");

  Serial.println("Status: SPACE AVAILABLE");

  Serial.println("Existing LED: ON");

  Serial.println("Full LED: OFF");

  Serial.println("Buzzer: OFF");

  Serial.println("================================");
}


// =================================================
// SPACE AVAILABLE
// =================================================

void spaceAvailable() {

  // Existing LED ON
  digitalWrite(LED_PIN, HIGH);

  // Full LED OFF
  digitalWrite(FULL_LED_PIN, LOW);

  Serial.println("SPACE AVAILABLE");

}


// =================================================
// QUEUE FULL
// =================================================

void queueFull() {

  // Existing LED OFF
  digitalWrite(LED_PIN, LOW);

  // New FULL LED ON
  digitalWrite(FULL_LED_PIN, HIGH);

  Serial.println("================================");
  Serial.println(" QUEUE FULL - 5 / 5 ");
  Serial.println(" ENTRY BLOCKED ");
  Serial.println("================================");

}


// =================================================
// BUZZER
// =================================================

void beepBuzzer() {

  Serial.println("BUZZER: ENTRY ATTEMPT BLOCKED");

  digitalWrite(BUZZER_PIN, HIGH);

  // Buzzer ON for 1 second
  delay(1000);

  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("BUZZER: OFF");

}


// =================================================
// ENTRY SENSOR
// =================================================

void checkEntry() {

  byte currentState = digitalRead(ENTRY_SENSOR);


  // Detect a NEW object/person

  if (currentState == SENSOR_DETECTED &&
      lastEntryState != SENSOR_DETECTED) {


    // ---------------------------------------------
    // SPACE AVAILABLE
    // ---------------------------------------------

    if (occupancy < MAX_CAPACITY) {

      occupancy++;


      Serial.print("ENTRY -> Occupancy: ");

      Serial.print(occupancy);

      Serial.println(" / 5");


      // -------------------------------------------
      // CAPACITY REACHED
      // -------------------------------------------

      if (occupancy >= MAX_CAPACITY) {

        occupancy = MAX_CAPACITY;

        queueFull();

      }


      // -------------------------------------------
      // SPACE STILL AVAILABLE
      // -------------------------------------------

      else {

        spaceAvailable();

        Serial.print("Spaces available: ");

        Serial.println(
          MAX_CAPACITY - occupancy
        );

      }

    }


    // ---------------------------------------------
    // ALREADY FULL
    // ---------------------------------------------

    else {

      // Make sure occupancy never goes above 5

      occupancy = MAX_CAPACITY;


      Serial.println(
        "ENTRY ATTEMPT - QUEUE ALREADY FULL"
      );


      // Keep FULL LED ON

      queueFull();


      // Beep for 1 second

      beepBuzzer();

    }

  }


  // Remember current sensor state

  lastEntryState = currentState;

}


// =================================================
// EXIT SENSOR
// =================================================

void checkExit() {

  byte currentState = digitalRead(EXIT_SENSOR);


  // Detect a NEW object/person

  if (currentState == SENSOR_DETECTED &&
      lastExitState != SENSOR_DETECTED) {


    // ---------------------------------------------
    // IF PEOPLE ARE INSIDE
    // ---------------------------------------------

    if (occupancy > 0) {

      occupancy--;


      Serial.print("EXIT -> Occupancy: ");

      Serial.print(occupancy);

      Serial.println(" / 5");


      // -------------------------------------------
      // SPACE AVAILABLE AGAIN
      // -------------------------------------------

      spaceAvailable();


      Serial.print("Spaces available: ");

      Serial.println(
        MAX_CAPACITY - occupancy
      );

    }


    // ---------------------------------------------
    // ALREADY EMPTY
    // ---------------------------------------------

    else {

      Serial.println("QUEUE EMPTY");

    }

  }


  // Remember current sensor state

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