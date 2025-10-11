/*
  ESP32 Server_Final.ino
  - Creates Wi-Fi AP (RoomMonitor)
  - Hosts web UI at http://192.168.4.1/
  - Accepts client POSTs at /updateClient
  - Returns JSON (message + thresholds) to client
  - Web UI allows login, view both rooms, set thresholds (persisted), send messages
  - LCD shows H/T/A on line1 and scrolling message on line2 (or EVACUATE on alarm)
  - Buzzer short beep every update; continuous when alarm active
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ135_PIN 34
#define BUZZER_PIN 13

// ---------------- CONFIG ----------------
const char* AP_SSID = "RoomMonitor";
const char* AP_PASS = "12345678";
const char* WEB_PASSWORD = "status@V!t";

const uint32_t POLL_INTERVAL_MS = 5000; // 5s
const uint32_t SCROLL_MS = 300;         // moderate scroll speed (for Line 2)

// default thresholds (used until overridden via web UI)
float defaultTemp = 32.0;
float defaultHum  = 55.0;
int   defaultAir  = 3000; // ADC units

// ---------------- HW / libs ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);
Preferences prefs;

// ---------------- Room structures ----------------
struct Room {
  float temp = 0;
  float hum = 0;
  int air = 0;
  String airState = "G"; // "G","A","B"
  unsigned long lastUpdate = 0;
  String message = "";
  bool alarm = false;
};
Room room1; // server local
Room room2; // client

// thresholds (persisted)
float thresholdTemp;
float thresholdHum;
int thresholdAir;

// scrolling
unsigned long lastScroll = 0;
int scrollPos = 0;

// timing
unsigned long lastSensorMillis = 0;

// ---------------- helpers ----------------
int readMQAverage(int pin, int samples = 4) {
  long sum = 0;
  for (int i=0;i<samples;i++) {
    sum += analogRead(pin);
    delay(6);
  }
  return (int)(sum / samples);
}

String airStateFromVal(int v) {
  if (v < 1500) return "G";
  if (v <= 3000) return "A";
  return "B";
}

void beepShort() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(70);
  digitalWrite(BUZZER_PIN, LOW);
}

// ---------------- HTTP / WEB UI (inline) ----------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Room Monitor</title>
<style>
body{font-family:system-ui,Arial;background:#f3f7fb;color:#0f172a;margin:12px}
.card{background:#fff;border-radius:10px;padding:12px;margin-bottom:12px;box-shadow:0 6px 20px rgba(2,6,23,0.06)}
h1{font-size:18px;margin:4px}
.row{display:flex;gap:12px;flex-wrap:wrap}
.metric{flex:1;min-width:160px}
.badge{display:inline-block;padding:4px 8px;border-radius:999px;font-weight:700}
.g{background:#ecfdf5;color:#065f46}.a{background:#fff7ed;color:#92400e}.b{background:#fff1f2;color:#7f1d1d}
label{display:block;margin-top:8px}
input,textarea,select{width:100%;padding:8px;border-radius:6px;border:1px solid #e6edf3}
button{background:#0b69ff;color:white;border:none;padding:8px 12px;border-radius:8px;cursor:pointer}
.small{color:#64748b;font-size:13px}
</style>
</head><body>
<div class="card"><h1>Room Monitor (AP)</h1>
<div id="loginDiv">
  <label>Password: <input id="pw" type="password"></label>
  <div style="margin-top:8px"><button onclick="login()">Login</button></div>
  <div id="lm" class="small"></div>
</div>
<div id="app" style="display:none">
  <div class="row">
    <div class="card metric"><h3>Room 1 (Server)</h3><div id="r1">Loading…</div></div>
    <div class="card metric"><h3>Room 2 (Client)</h3><div id="r2">Loading…</div></div>
  </div>

  <div class="card">
    <h3>Controls</h3>
    <div style="display:flex;gap:10px;flex-wrap:wrap">
      <div style="flex:1;min-width:250px">
        <label>Message (max 120 chars):<textarea id="msg" rows="2"></textarea></label>
        <label>Target:<select id="target"><option value="1">Room 1</option><option value="2">Room 2</option><option value="both">Both</option></select></label>
        <div style="margin-top:8px"><button onclick="sendMsg()">Send</button><span id="msgRes" class="small" style="margin-left:10px"></span></div>
      </div>

      <div style="flex:1;min-width:220px">
        <label>Temp threshold (°C): <input id="thTemp" type="number" step="0.1"></label>
        <label>Hum threshold (%): <input id="thHum" type="number" step="0.1"></label>
        <label>Air threshold (ADC): <input id="thAir" type="number" step="1"></label>
        <div style="margin-top:8px"><button onclick="saveThr()">Save thresholds</button><span id="thrRes" class="small" style="margin-left:10px"></span></div>
      </div>
    </div>
  </div>

  <div class="card"><h3>Raw</h3><pre id="raw"></pre></div>
</div>
</div>

<script>
const PW = '%PW%';
let logged=false;
async function login(){
  const v = document.getElementById('pw').value;
  if(v===PW){ logged=true; document.getElementById('loginDiv').style.display='none'; document.getElementById('app').style.display='block'; load(); setInterval(load, 5000);}
  else document.getElementById('lm').innerText='Wrong password';
}
async function load(){
  try {
    const r = await fetch('/status');
    if(!r.ok) return;
    const j = await r.json();
    const r1 = j.room1, r2 = j.room2;
    document.getElementById('r1').innerHTML =
      'Temp: ' + r1.temp.toFixed(1) + '°C<br>Hum: ' + r1.hum.toFixed(1) + '%<br>Air: ' + r1.air + ' <span class="badge ' + (r1.airState==='G'?'g':r1.airState==='A'?'a':'b') + '">' + r1.airState + '</span><div class="small">Last: ' + new Date(r1.lastUpdate).toLocaleTimeString() + '</div><div class="small">Msg: ' + (j.messageRoom1||'<i>none</i>') + '</div>';
    document.getElementById('r2').innerHTML =
      'Temp: ' + r2.temp.toFixed(1) + '°C<br>Hum: ' + r2.hum.toFixed(1) + '%<br>Air: ' + r2.air + ' <span class="badge ' + (r2.airState==='G'?'g':r2.airState==='A'?'a':'b') + '">' + r2.airState + '</span><div class="small">Last: ' + new Date(r2.lastUpdate).toLocaleTimeString() + '</div><div class="small">Msg: ' + (j.messageRoom2||'<i>none</i>') + '</div>';
    document.getElementById('thTemp').value = j.thresholds.temp;
    document.getElementById('thHum').value = j.thresholds.hum;
    document.getElementById('thAir').value = j.thresholds.air;
    document.getElementById('raw').innerText = JSON.stringify(j, null, 2);
  } catch(e) { console.error(e); }
}
async function sendMsg(){
  const payload = { msg: document.getElementById('msg').value, target: document.getElementById('target').value };
  const r = await fetch('/sendMsg', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify(payload)});
  document.getElementById('msgRes').innerText = r.ok ? 'Sent' : 'Failed';
}
async function saveThr(){
  const payload = { temp: parseFloat(document.getElementById('thTemp').value), hum: parseFloat(document.getElementById('thHum').value), air: parseInt(document.getElementById('thAir').value) };
  const r = await fetch('/setThr', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify(payload)});
  document.getElementById('thrRes').innerText = r.ok ? 'Saved' : 'Failed';
}
</script>
</body></html>
)rawliteral";

void handleRoot() {
  String page = INDEX_HTML;
  page.replace("%PW%", String(WEB_PASSWORD));
  server.send(200, "text/html", page);
}

void handleStatus() {
  StaticJsonDocument<512> doc;
  JsonObject r1 = doc.createNestedObject("room1");
  r1["temp"] = room1.temp;
  r1["hum"] = room1.hum;
  r1["air"] = room1.air;
  r1["airState"] = room1.airState;
  r1["lastUpdate"] = room1.lastUpdate;

  JsonObject r2 = doc.createNestedObject("room2");
  r2["temp"] = room2.temp;
  r2["hum"] = room2.hum;
  r2["air"] = room2.air;
  r2["airState"] = room2.airState;
  r2["lastUpdate"] = room2.lastUpdate;

  doc["messageRoom1"] = room1.message;
  doc["messageRoom2"] = room2.message;

  JsonObject thr = doc.createNestedObject("thresholds");
  thr["temp"] = thresholdTemp;
  thr["hum"] = thresholdHum;
  thr["air"] = thresholdAir;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

// Client posts here: server responds with message + thresholds
void handleUpdateClient() {
  StaticJsonDocument<256> req;
  if (server.method() == HTTP_POST && server.hasArg("plain")) {
    DeserializationError err = deserializeJson(req, server.arg("plain"));
    if (err) {
      server.send(400, "text/plain", "Bad JSON");
      return;
    }
  } else {
    server.send(400, "text/plain", "POST JSON required");
    return;
  }

  // parse
  float t = req["temperature"] | 0.0;
  float h = req["humidity"] | 0.0;
  int a = req["air"] | 0;

  room2.temp = t;
  room2.hum = h;
  room2.air = a;
  room2.airState = airStateFromVal(a);
  room2.lastUpdate = millis();
  room2.alarm = (room2.temp > thresholdTemp) || (room2.hum > thresholdHum) || (room2.air > thresholdAir);

  // respond with message & thresholds (so client can display and update its thresholds)
  StaticJsonDocument<256> res;
  res["message"] = room2.message;
  JsonObject thr = res.createNestedObject("thresholds");
  thr["temp"] = thresholdTemp;
  thr["hum"] = thresholdHum;
  thr["air"] = thresholdAir;
  String out;
  serializeJson(res, out);
  server.send(200, "application/json", out);
}

void handleSendMsg() {
  if (server.method() != HTTP_POST) { server.send(405, "text/plain", "POST only"); return; }
  StaticJsonDocument<256> js;
  DeserializationError err = deserializeJson(js, server.arg("plain"));
  if (err) { server.send(400, "text/plain", "Bad JSON"); return; }
  String msg = js["msg"] | "";
  String target = js["target"] | "both";
  if (target == "1") room1.message = msg;
  else if (target == "2") room2.message = msg;
  else { room1.message = msg; room2.message = msg; }
  server.send(200, "text/plain", "OK");
}

void handleSetThr() {
  if (server.method() != HTTP_POST) { server.send(405, "text/plain", "POST only"); return; }
  StaticJsonDocument<128> js;
  DeserializationError err = deserializeJson(js, server.arg("plain"));
  if (err) { server.send(400, "text/plain", "Bad JSON"); return; }
  thresholdTemp = js["temp"] | thresholdTemp;
  thresholdHum  = js["hum"]  | thresholdHum;
  thresholdAir  = js["air"]  | thresholdAir;

  // persist thresholds
  prefs.putFloat("thTemp", thresholdTemp);
  prefs.putFloat("thHum", thresholdHum);
  prefs.putInt("thAir", thresholdAir);

  // recalc alarms
  room1.alarm = (room1.temp > thresholdTemp) || (room1.hum > thresholdHum) || (room1.air > thresholdAir);
  room2.alarm = (room2.temp > thresholdTemp) || (room2.hum > thresholdHum) || (room2.air > thresholdAir);

  server.send(200, "text/plain", "OK");
}

// ---------------- SENSOR DATA UPDATE ----------------
void updateLocalSensors() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int a = readMQAverage(MQ135_PIN, 4);
  if (!isnan(h)) room1.hum = h;
  if (!isnan(t)) room1.temp = t;
  room1.air = a;
  room1.airState = airStateFromVal(a);
  room1.lastUpdate = millis();
  room1.alarm = (room1.temp > thresholdTemp) || (room1.hum > thresholdHum) || (room1.air > thresholdAir);
}

// ---------------- LCD UPDATE FUNCTIONS ----------------

// Updates Line 1 (Static Sensor Data) and controls the continuous alarm buzzer.
// This runs once every POLL_INTERVAL_MS (5s).
void updateStaticData() {
  bool anyAlarm = room1.alarm || room2.alarm;

  // Line 1: Update Room 1 sensor data
  char line1[17];
  snprintf(line1, 17, "H=%02d T=%02d A=%s", (int)room1.hum, (int)room1.temp, room1.airState.c_str());
  lcd.setCursor(0,0); 
  lcd.print(line1);

  // Control the continuous buzzer
  if (anyAlarm) {
    digitalWrite(BUZZER_PIN, HIGH); // continuous
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// Handles the non-blocking scrolling for Line 2 or displays the alarm message.
// This runs continuously in the main loop, checked against SCROLL_MS (300ms).
void updateScrollLine2() {
  bool anyAlarm = room1.alarm || room2.alarm;

  // Alarm state overrides message/scrolling on Line 2
  if (anyAlarm) {
    lcd.setCursor(0,1);
    lcd.print("!! EVACUATE - ALERT");
    return;
  } 

  // Prefer room1 message, else room2
  String msg = room1.message.length() ? room1.message : room2.message;

  if (msg.length() == 0 || msg.length() <= 16) {
    // Static display for short or no message
    lcd.setCursor(0, 1);
    lcd.print(msg);
    for(int i = msg.length(); i < 16; i++) lcd.print(" "); // Padding to clear old text
    scrollPos = 0; // Reset scroll counter when static
    return;
  }

  // Scrolling logic (Non-blocking timing)
  unsigned long now = millis();
  if (now - lastScroll >= SCROLL_MS) {
    lastScroll = now;
    
    scrollPos++;
    if (scrollPos >= (int)msg.length()) {
      scrollPos = 0; // Wrap around
    }

    String display = msg.substring(scrollPos);
    
    // Wrap-around implementation: pad with spaces and wrap the beginning of the message
    if (display.length() < 16) {
      display += "   " + msg.substring(0, min((int)msg.length(), 16 - (int)display.length()));
    }
    
    // Print the final 16 characters to line 2
    lcd.setCursor(0, 1);
    lcd.print(display.substring(0, 16));
  }
}

// ---------------- SETUP & LOOP ----------------
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init(); lcd.backlight();
  dht.begin();
  prefs.begin("roommon", false);
  lcd.clear();

  // load thresholds (persisted) or defaults
  thresholdTemp = prefs.getFloat("thTemp", defaultTemp);
  thresholdHum  = prefs.getFloat("thHum", defaultHum);
  thresholdAir  = prefs.getInt("thAir", defaultAir);

  // start AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/updateClient", HTTP_POST, handleUpdateClient);
  server.on("/sendMsg", HTTP_POST, handleSendMsg);
  server.on("/setThr", HTTP_POST, handleSetThr);
  server.begin();
  
  // Force an immediate first read and display update
  lastSensorMillis = millis() - POLL_INTERVAL_MS; 
}

void loop() {
  server.handleClient();
  unsigned long now = millis();
  
  // 1. Sensor/Data Update (Slower, every 5s)
  if (now - lastSensorMillis >= POLL_INTERVAL_MS) {
    lastSensorMillis = now;
    updateLocalSensors();
    
    // short beep if no continuous alarm is active
    if (!(room1.alarm || room2.alarm)) beepShort();
    
    // Update Line 1 and continuous buzzer state
    updateStaticData(); 
  }
  
  // 2. LCD Scroll Update (Faster, non-blocking, always checking)
  updateScrollLine2();
}
