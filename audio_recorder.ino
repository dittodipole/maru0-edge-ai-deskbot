#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP_I2S.h>
#include "secrets.h"

#define I2S_SCK 4
#define I2S_WS  5
#define I2S_SD  6
#define BUTTON  0          // the BOOT button

const char* serverUrl = "http://10.202.148.200:5000/audio";  // check the IP Flask prints!

const int SAMPLE_RATE = 16000;
const int SECONDS = 2;
const int NUM_SAMPLES = SAMPLE_RATE * SECONDS;   // 32,000 samples

I2SClass i2s;
int32_t* raw;    // 32-bit recording (in PSRAM)
int16_t* pcm;    // 16-bit version to send (in PSRAM)

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON, INPUT_PULLUP);

  raw = (int32_t*) ps_malloc(NUM_SAMPLES * sizeof(int32_t));
  pcm = (int16_t*) ps_malloc(NUM_SAMPLES * sizeof(int16_t));
  if (!raw || !pcm) {
    Serial.println("PSRAM allocation failed");
    while (true) delay(1000);
  }

  i2s.setPins(I2S_SCK, I2S_WS, -1, I2S_SD);
  if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT,
                 I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
    Serial.println("I2S init failed");
    while (true) delay(1000);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nReady. Press BOOT to record.");
}

void record() {
  rgbLedWrite(RGB_BUILTIN, 40, 0, 0);   // red = recording

  // 1. Fill the buffer block by block
  // Throw away the first ~100 ms (3 blocks × 512 samples ≈ 1,536 samples)
  for (int i = 0; i < 3; i++) {
    i2s.readBytes((char*)raw, 512 * 4);
  }

  size_t total = 0;
  while (total < NUM_SAMPLES) {
    size_t want = min((size_t)512, NUM_SAMPLES - total);
    size_t got = i2s.readBytes((char*)(raw + total), want * 4);
    total += got / 4;
  }

  // 2. 24-bit values + average (offset) over the whole recording
  int64_t sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    raw[i] = raw[i] >> 8;
    sum += raw[i];
  }
  int32_t offset = sum / NUM_SAMPLES;

  // 3. Convert to 16-bit
  for (int i = 0; i < NUM_SAMPLES; i++) {
    int32_t s = (raw[i] - offset) >> 8;
    if (s > 32767) s = 32767;       // clamp: prevent overflow
    if (s < -32768) s = -32768;
    pcm[i] = s;
  }

  rgbLedWrite(RGB_BUILTIN, 0, 0, 40);   // blue = sending
}

void send() {
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/octet-stream");   // raw bytes
  int code = http.POST((uint8_t*)pcm, NUM_SAMPLES * sizeof(int16_t));
  http.end();

  Serial.printf("Sent %d bytes, server replied: %d\n", NUM_SAMPLES * 2, code);
  rgbLedWrite(RGB_BUILTIN, 0, code == 200 ? 40 : 0, 0);   // green = success
}

void loop() {
  if (digitalRead(BUTTON) == LOW) {
    record();
    send();
    while (digitalRead(BUTTON) == LOW) delay(10);   // wait until released
  }
}