"""
SMART QUEUE SYSTEM - PC BRIDGE
--------------------------------
Reads status updates from the Arduino Uno over USB serial, then:
  1. Serves a live local web dashboard (works on your local WiFi/network)
  2. Pushes the same data to Adafruit IO (works from anywhere online)

SETUP:
  1. Install Python 3 if you don't have it: https://www.python.org/downloads/
  2. Install the required packages by running this in a terminal/command prompt:
       pip install pyserial flask requests
  3. Edit the CONFIG section below:
       - SERIAL_PORT: the COM port your Arduino Uno is on (check Arduino IDE
         under Tools > Port, e.g. "COM5" on Windows, "/dev/ttyUSB0" on Linux/Mac)
       - AIO_USERNAME / AIO_KEY: from https://io.adafruit.com (key icon, top right)
         Leave these as-is if you only want the local dashboard for now.
  4. Run it:
       python bridge_dashboard.py
  5. Keep the Arduino Uno plugged into this computer via USB while running.
  6. Open the local dashboard at the address printed in the terminal
     (something like http://192.168.1.42:5000)
"""

import serial
import threading
import time
import json

from flask import Flask, jsonify, render_template_string

try:
    import requests
    REQUESTS_AVAILABLE = True
except ImportError:
    REQUESTS_AVAILABLE = False

# ------------- CONFIG -------------
SERIAL_PORT = "COM5"        # <-- change this to match your Arduino's port
BAUD_RATE = 9600

# Adafruit IO (for the ONLINE dashboard). Leave as-is to skip cloud push.
AIO_USERNAME = "YOUR_ADAFRUIT_IO_USERNAME"
AIO_KEY = "YOUR_ADAFRUIT_IO_AIO_KEY"
PUSH_TO_ADAFRUIT = False   # set True once you've filled in the two lines above

FEED_OCCUPANCY = "queue-occupancy"
FEED_SLOTS_LEFT = "queue-slots-left"
FEED_GATE = "queue-gate-status"
# -----------------------------------

state_lock = threading.Lock()
state = {"occupancy": 0, "max": 5, "slotsLeft": 5, "gateOpen": True}


def push_to_adafruit_io(feed_key, value):
    if not (PUSH_TO_ADAFRUIT and REQUESTS_AVAILABLE):
        return
    url = f"https://io.adafruit.com/api/v2/{AIO_USERNAME}/feeds/{feed_key}/data"
    headers = {"X-AIO-Key": AIO_KEY, "Content-Type": "application/json"}
    try:
        requests.post(url, headers=headers, json={"value": str(value)}, timeout=5)
    except Exception as e:
        print(f"[Adafruit IO] push to {feed_key} failed: {e}")


def serial_reader():
    """Runs in the background, continuously reading lines from the Arduino."""
    while True:
        try:
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
                print(f"Connected to Arduino on {SERIAL_PORT}")
                while True:
                    line = ser.readline().decode("utf-8", errors="ignore").strip()
                    if not line.startswith("STATUS,"):
                        continue
                    # Format: STATUS,<occupancy>,<max>,<OPEN/CLOSED>
                    parts = line.split(",")
                    if len(parts) != 4:
                        continue
                    _, occ_str, max_str, gate_str = parts
                    occ = int(occ_str)
                    mx = int(max_str)
                    gate_open = (gate_str == "OPEN")

                    with state_lock:
                        state["occupancy"] = occ
                        state["max"] = mx
                        state["slotsLeft"] = mx - occ
                        state["gateOpen"] = gate_open

                    print(f"Updated: occupancy={occ}, slotsLeft={mx - occ}, gate={gate_str}")

                    push_to_adafruit_io(FEED_OCCUPANCY, occ)
                    push_to_adafruit_io(FEED_SLOTS_LEFT, mx - occ)
                    push_to_adafruit_io(FEED_GATE, gate_str)
        except serial.SerialException as e:
            print(f"Serial connection lost or unavailable ({e}). Retrying in 3s...")
            time.sleep(3)


# ---------- LOCAL WEB DASHBOARD ----------
app = Flask(__name__)

INDEX_HTML = """
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
  <div class="sub">Live occupancy monitoring (via Arduino Uno bridge)</div>
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
"""


@app.route("/")
def index():
    return render_template_string(INDEX_HTML)


@app.route("/status")
def status():
    with state_lock:
        return jsonify(state)


if __name__ == "__main__":
    reader_thread = threading.Thread(target=serial_reader, daemon=True)
    reader_thread.start()

    print("Starting local dashboard at http://<your-computer-ip>:5000")
    print("Find your computer's local IP with 'ipconfig' (Windows) or 'ifconfig' (Mac/Linux)")
    app.run(host="0.0.0.0", port=5000)
