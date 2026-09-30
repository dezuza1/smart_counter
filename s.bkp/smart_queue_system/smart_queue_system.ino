/*
  SMART QUEUE MANAGEMENT SYSTEM
  ------------------------------
  Hardware: ESP32, 2x IR obstacle sensors, 1x SG90 servo motor

  HOW IT WORKS:
  - Entry IR sensor -> increases occupancy count
  - Exit IR sensor   -> decreases occupancy count
  - When occupancy reaches MAX_CAPACITY, servo closes the gate
  - When occupancy drops below MAX_CAPACITY, servo opens the gate
  - A live web dashboard (hosted BY the ESP32 itself) shows occupancy,
    slots left, and gate status. It refreshes every second automatically.

  LIBRARIES NEEDED (install via Arduino IDE > Tools > Manage Libraries):
  - "ESP32Servo" by Kevin Harrington  (search "ESP32Servo")
  - WiFi.h and WebServer.h come built-in with the ESP32 board package.

  BOARD SETUP:
  - Install "ESP32 by Espressif Systems" in Boards Manager if you haven't.
  - Select your ESP32 board under Tools > Board.
*/

#include <WiFi.h>
#include <WebServer.h>
//#include <ESP32Servo.h>

// ------------- CONFIG -------------
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const int MAX_CAPACITY = 5;

const int ENTRY_SENSOR_PIN = 27;   // IR sensor at entry
const int EXIT_SENSOR_PIN  = 26;   // IR sensor at exit
const int SERVO_PIN        = 13;   // Servo signal pin

const int GATE_OPEN_ANGLE   = 90;
const int GATE_CLOSED_ANGLE = 0;

// Most IR obstacle sensor modules output LOW when they detect an object.
// If your sensor works the opposite way, just flip this constant to HIGH.
const int SENSOR_TRIGGERED = LOW;

// Minimum time between two counts on the SAME sensor, to avoid one person
// being counted multiple times while walking through the beam (ms).
const unsigned long DEBOUNCE_MS = 1200;
// -----------------------------------

Servo gateServo;
WebServer server(80);

int occupancy = 0;
bool gateOpen = true;

unsigned long lastEntryTrigger = 0;
unsigned long lastExitTrigger  = 0;

void setGate(bool open) {
  gateOpen = open;
  gateServo.write(open ? GATE_OPEN_ANGLE : GATE_CLOSED_ANGLE);
}

void handleEntry() {
  if (digitalRead(ENTRY_SENSOR_PIN) == SENSOR_TRIGGERED) {
    unsigned long now = millis();
    if (now - lastEntryTrigger > DEBOUNCE_MS) {
      lastEntryTrigger = now;
      if (occupancy < MAX_CAPACITY) {
        occupancy++;
        Serial.printf("Entry detected. Occupancy = %d\n", occupancy);
        if (occupancy >= MAX_CAPACITY) {
          setGate(false); // close gate, counter full
          Serial.println("Counter FULL. Gate closed.");
        }
      } else {
        Serial.println("Entry blocked: counter already full.");
      }
    }
  }
}

void handleExit() {
  if (digitalRead(EXIT_SENSOR_PIN) == SENSOR_TRIGGERED) {
    unsigned long now = millis();
    if (now - lastExitTrigger > DEBOUNCE_MS) {
      lastExitTrigger = now;
      if (occupancy > 0) {
        occupancy--;
        Serial.printf("Exit detected. Occupancy = %d\n", occupancy);
        if (occupancy < MAX_CAPACITY && !gateOpen) {
          setGate(true); // reopen gate, space available again
          Serial.println("Space available. Gate opened.");
        }
      }
    }
  }
}

// ---------- WEB DASHBOARD ----------

// This HTML/CSS/JS lives entirely on the ESP32 and is sent to any browser
// that visits the ESP32's IP address. It polls /status every second.
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Queue Dashboard</title>
<style>
  body { font-family: Arial, sans-serif; background:#0f172a; color:#e2e8f0;
         display:flex; flex-direction:column; align-items:center; padding:40px; margin:0; }
  h1 { margin-bottom: 4px; }
  .sub { color:#94a3b8; margin-bottom:30px; }
  .card { background:#1e293b; border-radius:16px; padding:30px 50px;
          text-align:center; box-shadow:0 4px 20px rgba(0,0,0,0.4); }
  .big { font-size:64px; font-weight:bold; margin:10px 0; }
  .label { color:#94a3b8; font-size:14px; text-transform:uppercase; letter-spacing:1px;}
  .status { margin-top:20px; padding:10px 20px; border-radius:20px; font-weight:bold; display:inline-block;}
  .open { background:#16a34a33; color:#4ade80; }
  .closed { background:#dc262633; color:#f87171; }
  .bar-bg { background:#334155; border-radius:10px; height:14px; width:280px; margin:16px auto; overflow:hidden;}
  .bar-fill { background:#38bdf8; height:100%; transition:width 0.4s ease; }
</style>
</head>
<body>
  <h1>Smart Queue Dashboard</h1>
  <div class="sub">Live occupancy monitoring</div>
  <div class="card">
    <div class="label">Current Occupancy</div>
    <div class="big" id="occ">--</div>
    <div class="bar-bg"><div class="bar-fill" id="bar" style="width:0%"></div></div>
    <div class="label" id="left">-- slots left</div>
    <div class="status" id="gate">--</div>
  </div>
<script>
async function refresh() {
  try {
    const res = await fetch('/status');
    const data = await res.json();
    document.getElementById('occ').textContent = data.occupancy + ' / ' + data.max;
    document.getElementById('left').textContent = data.slotsLeft + ' slot(s) left';
    const pct = (data.occupancy / data.max) * 100;
    document.getElementById('bar').style.width = pct + '%';
    const gateEl = document.getElementById('gate');
    if (data.gateOpen) {
      gateEl.textContent = 'GATE OPEN';
      gateEl.className = 'status open';
    } else {
      gateEl.textContent = 'GATE CLOSED - FULL';
      gateEl.className = 'status closed';
    }
  } catch (e) { console.log('fetch failed', e); }
}
setInterval(refresh, 1000);
refresh();
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  String json = "{";
  json += "\"occupancy\":" + String(occupancy) + ",";
  json += "\"max\":" + String(MAX_CAPACITY) + ",";
  json += "\"slotsLeft\":" + String(MAX_CAPACITY - occupancy) + ",";
  json += "\"gateOpen\":" + String(gateOpen ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

// ------------------------------------

void setup() {
  Serial.begin(115200);

  pinMode(ENTRY_SENSOR_PIN, INPUT);
  pinMode(EXIT_SENSOR_PIN, INPUT);

  gateServo.setPeriodHertz(50);
  gateServo.attach(SERVO_PIN, 500, 2400);
  setGate(true); // start with gate open

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! Dashboard available at: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.begin();
}

void loop() {
  handleEntry();
  handleExit();
  server.handleClient();
}
