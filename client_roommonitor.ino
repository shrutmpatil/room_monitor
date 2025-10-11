#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ135PIN 34
#define BUZZER 13

// --- HW Setup ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

// === Wi-Fi credentials (must match server AP) ===
const char* ssid     = "RoomMonitor";
const char* password = "12345678";
const char* serverIP = "192.168.4.1";
const char* updatePath = "/updateClient"; // Server path for client updates

// === Sensor + system variables ===
float temperature = 0, humidity = 0;
int airQuality = 0;
String messageRoom2 = "";

// --- Timing ---
unsigned long lastRead = 0;

// === Air-quality bands (Client side display logic) ===
// 'airGood' is static (G/A transition). 'airAverage' (A/B transition) is updated by the server's threshold.
int airGood = 1500;
int airAverage = 3000;

// === LCD scroll control (Non-blocking) ===
int scrollDelay = 300;  // moderate scroll speed, matches server
unsigned long lastScroll = 0;
int scrollPos = 0;

// === Determine air-quality label (G, A, B) ===
// Uses the dynamically updated 'airAverage' threshold for the 'Bad' category.
String getAirQualityLabel(int air) {
  if (air < airGood) return "G";      // Good (below 1500)
  else if (air < airAverage) return "A"; // Average (between 1500 and threshold)
  else return "B";                     // Bad (above or equal to threshold)
}

// === Updates the static first line of the LCD ===
void updateLCDLine1() {
  lcd.setCursor(0, 0);
  // Using snprintf to ensure fixed 16-char length and cleaner formatting
  char line1[17];
  snprintf(line1, 17, "H=%02d T=%02d A=%s",
    (int)humidity,
    (int)temperature,
    getAirQualityLabel(airQuality).c_str());
  lcd.print(line1);
}

// === Non-blocking LCD message scroller (for line 2) ===
void updateScroll() {
  unsigned long now = millis();
  if (now - lastScroll >= scrollDelay) {
    lastScroll = now;

    if (messageRoom2.length() <= 16) {
      // Static display for short message or "none"
      lcd.setCursor(0, 1);
      lcd.print(messageRoom2);
      // Pad with spaces to clear any previous longer message
      for(int i = messageRoom2.length(); i < 16; i++) lcd.print(" ");
      scrollPos = 0;
      return;
    }
   
    // Scrolling logic
    scrollPos++;
    if (scrollPos >= (int)messageRoom2.length()) {
      scrollPos = 0;
    }

    String display = messageRoom2.substring(scrollPos);
   
    // Wrap-around implementation
    if (display.length() < 16) {
      // Append start of message to wrap around
      display += "   " + messageRoom2.substring(0, min((int)messageRoom2.length(), 16 - (int)display.length()));
    }

    lcd.setCursor(0, 1);
    lcd.print(display.substring(0, 16));
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(MQ135PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  lcd.init();
  lcd.backlight();
  dht.begin();

  // --- Wi-Fi connect ---
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  lcd.clear();
  lcd.print("WiFi Connected");
  delay(1000);
  lcd.clear();
}

void loop() {
  unsigned long now = millis();

  // === Read sensors and update every 5 seconds ===
  if (now - lastRead >= 5000) {
    lastRead = now;
   
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    airQuality = analogRead(MQ135PIN);

    // --- Beep once on every refresh (sync with server) ---
    tone(BUZZER, 2000, 250);

    // --- Send readings to server (POST JSON) ---
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
     
      // 1. Prepare JSON payload
      StaticJsonDocument<128> reqDoc;
      reqDoc["temperature"] = temperature;
      reqDoc["humidity"] = humidity;
      reqDoc["air"] = airQuality;
     
      String jsonPayload;
      serializeJson(reqDoc, jsonPayload);
     
      // 2. Configure POST request
      String url = "http://" + String(serverIP) + updatePath;
      http.begin(url);
      http.addHeader("Content-Type", "application/json"); // Required for server to parse body

      int httpCode = http.POST(jsonPayload);

      if (httpCode == HTTP_CODE_OK) { // HTTP_CODE_OK is 200
        String payload = http.getString();
        Serial.println("Response: " + payload);

        // 3. Process JSON response
        StaticJsonDocument<256> resDoc;
        DeserializationError err = deserializeJson(resDoc, payload);
       
        if (!err) {
          // Extract message for LCD line 2
          const char* msgPtr = resDoc["message"];
          messageRoom2 = msgPtr ? String(msgPtr) : "";
         
          // Extract new Air threshold from server to align client's 'B' category
          airAverage = resDoc["thresholds"]["air"] | airAverage;
        } else {
          Serial.print("JSON Deserialization failed: ");
          Serial.println(err.c_str());
        }
      } else {
        Serial.printf("HTTP POST failed, code: %d\n", httpCode);
      }
      http.end();
    }

    // --- LCD Line 1 (Static display update) ---
    updateLCDLine1();
    // Reset scroll position so new message (if any) starts fresh on line 2
    scrollPos = 0;
  }

  // --- LCD Line 2 (Continuous scroll update) ---
  updateScroll();

  delay(50);
}