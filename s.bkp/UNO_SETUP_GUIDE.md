# Smart Queue System — Arduino Uno + PC Bridge Version

Since the Uno has no WiFi, this version splits the work:
- **Arduino Uno**: reads the sensors, controls the servo gate, sends status over USB serial
- **Your PC** (via `bridge_dashboard.py`): reads that serial data, serves a live local dashboard, and optionally pushes to Adafruit IO for an online dashboard

Your PC needs to stay on and connected to the Uno via USB the whole time the system runs.

## 1. Wiring (same idea as before, different pins)
- Entry IR sensor OUT → **Pin 2**
- Exit IR sensor OUT → **Pin 3**
- Servo signal → **Pin 9**
- LED anode (+, longer leg) → **Pin 8**, through a 220Ω resistor
- LED cathode (-, shorter leg) → **GND**
- Sensors: VCC → 5V, GND → GND
- Servo: red → 5V (external 5V supply recommended if it jitters), brown/black → GND, orange → Pin 9

**LED behavior:** glows when there's space available, turns off when the counter hits max (5).

## 2. Upload the Arduino sketch
1. Open `uno_queue_system.ino` in the Arduino IDE.
2. Select **Tools → Board → Arduino Uno**, and the correct COM port.
3. Click Upload. No extra libraries needed — the `Servo` library ships with the IDE.
4. Open the Serial Monitor (9600 baud) to confirm you see lines like:
   ```
   STATUS,0,5,OPEN
   ```
   whenever the sensors trigger. **Close the Serial Monitor once confirmed** — only one program can read the serial port at a time, and the Python bridge needs it next.

## 3. Set up the Python bridge
1. Install Python 3 if you don't already have it: https://www.python.org/downloads/
   (On the installer, tick "Add Python to PATH")
2. Open a terminal / command prompt and install the required packages:
   ```
   pip install pyserial flask requests
   ```
3. Open `bridge_dashboard.py` in any text editor and set:
   ```python
   SERIAL_PORT = "COM5"   # match whatever port you saw in the Arduino IDE
   ```
4. Run it:
   ```
   python bridge_dashboard.py
   ```
5. It will print something like:
   ```
   Connected to Arduino on COM5
   Starting local dashboard at http://<your-computer-ip>:5000
   ```
6. Find your PC's local IP (Windows: run `ipconfig`, look for "IPv4 Address"). Open `http://<that-ip>:5000` in a browser on your phone or another device on the same WiFi.

## 4. (Optional, skip for now) Online dashboard
You said you just want it running locally, so you can skip Adafruit IO entirely — leave `PUSH_TO_ADAFRUIT = False` in `bridge_dashboard.py` as it already is by default. The local dashboard at `http://<your-computer-ip>:5000` is all you need. If you ever want it accessible from outside your WiFi later, the steps are still in the earlier version of this guide.

## 5. Common issues
| Problem | Likely cause |
|---|---|
| `bridge_dashboard.py` says "could not open port" | Wrong `SERIAL_PORT`, or Serial Monitor/another program is still holding the port open — close it first |
| Dashboard shows nothing / stuck on "--" | Check the terminal running the script for "Connected to Arduino" — if missing, check wiring and COM port |
| Works locally but not online | Confirm `PUSH_TO_ADAFRUIT = True` and your username/key are correct; check the terminal for "[Adafruit IO] push failed" messages |
| Dashboard stops updating if you close the terminal | This is expected — the bridge script (and your PC) must stay running for the dashboard to work, since the Uno has no WiFi of its own |
| LED doesn't light up at all | Check polarity — the longer leg (anode) goes to Pin 8 side via the resistor, shorter leg (cathode) to GND. LEDs only work one way round |
| LED stays on even when full, or off even when free | Confirm it's wired to Pin 8 exactly, and check `LED_PIN` in the sketch matches your wiring |
