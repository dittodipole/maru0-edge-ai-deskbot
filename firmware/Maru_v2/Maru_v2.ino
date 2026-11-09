#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <secrets.h>

WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

const char* serverUrl = "http://10.202.148.200:5000/data";

#define DHTPIN 4
#define DHTTYPE DHT11
#define BUTTONPIN 18
#define PIRPIN 19
#define LIGHTPIN 34
#define SOUNDPIN 35

const unsigned long SEND_INTERVAL = 10000;  // send every 10 seconds

DHT dht(DHTPIN, DHTTYPE);

bool studying = false;
int lastButtonState = HIGH;
unsigned long lastPressTime = 0;
unsigned long lastSend = 0;

int soundMin = 4095;      // ESP32 ADC is 12-bit: 0 to 4095
int soundMax = 0;
bool motionSeen = false;  // was there motion at any point since the last send?

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(BUTTONPIN, INPUT_PULLUP);
  pinMode(PIRPIN, INPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // 1. Button: same pattern as weekend 1
  int buttonState = digitalRead(BUTTONPIN);
  if (buttonState == LOW && lastButtonState == HIGH && millis() - lastPressTime > 200) {
    studying = !studying;
    lastPressTime = millis();
    Serial.println(studying ? "Session started" : "Session stopped");
  }
  lastButtonState = buttonState;

  // 2. Sound and motion: watched continuously between sends
  int sound = analogRead(SOUNDPIN);
  if (sound < soundMin) soundMin = sound;
  if (sound > soundMax) soundMax = sound;
  if (digitalRead(PIRPIN) == HIGH) motionSeen = true;

  // 3. Every 10 seconds: read the rest, build JSON, send
  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int light = analogRead(LIGHTPIN);
    int loudness = soundMax - soundMin;

    String json = "{\"temp\":";
    json += isnan(temperature) ? String("null") : String(temperature, 1);
    json += ",\"hum\":";
    json += isnan(humidity) ? String("null") : String(humidity, 0);
    json += ",\"light\":";
    json += light;
    json += ",\"motion\":";
    json += motionSeen ? 1 : 0;
    json += ",\"sound\":";
    json += loudness;
    json += ",\"studying\":";
    json += studying ? 1 : 0;
    json += "}";

    Serial.println(json);

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");
      int code = http.POST(json);
      Serial.print("Server replied: ");
      Serial.println(code);
      http.end();
    }

    // reset the windows for the next 10 seconds
    soundMin = 4095;
    soundMax = 0;
    motionSeen = false;
  }
}
