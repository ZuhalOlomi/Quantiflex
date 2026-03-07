#include <WiFi.h>
#include <Wire.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#define RGB_BUILTIN 21

const char* ssid     = "UTBiome_Knee";
const char* password = "password123";
const int   IMU_PORT = 9000;

// ── Pins ──────────────────────────────────────────────────────────────────────
#define SDA_PIN 4
#define SCL_PIN 5
const int emg1Pin = 7;
const int emg2Pin = 8;
const int MPU     = 0x68;

// ── State ─────────────────────────────────────────────────────────────────────
volatile bool trackingActive = false;

// ── Thigh IMU (local) ─────────────────────────────────────────────────────────
const float CF_ALPHA  = 0.98;
float thighAngle      = 0.0;
float gyroOffset      = 0.0;
unsigned long lastIMUTime = 0;

// ── Shin IMU (from slave) ─────────────────────────────────────────────────────
float shinAngle       = 0.0;
float shinAnglePrev   = 0.0;
float shinAngleInterp = 0.0;
unsigned long lastShinReceived = 0;
unsigned long lastShinInterval = 20;
String slaveBuffer = "";

// ── Timing ────────────────────────────────────────────────────────────────────
unsigned long lastSendTime    = 0;
unsigned long lastCleanupTime = 0;
const unsigned long SEND_INTERVAL_MS    = 20;   // 50Hz
const unsigned long CLEANUP_INTERVAL_MS = 500;

// ── Servers ───────────────────────────────────────────────────────────────────
AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");
WiFiServer imuTcpServer(IMU_PORT);
WiFiClient slaveClient;

// ── LED feedback ──────────────────────────────────────────────────────────────
void updateHardwareFeedback() {
  if (trackingActive) {
    neopixelWrite(RGB_BUILTIN, 0, 64, 0);  // Green = recording
  } else {
    neopixelWrite(RGB_BUILTIN, 64, 0, 0);  // Red = idle
  }
}

// ── IMU helpers ───────────────────────────────────────────────────────────────
float getAccelAngle() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);
  int16_t ax = (Wire.read() << 8) | Wire.read();  // Must clock out
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();
  return atan2((float)ay, (float)az) * 180.0 / PI;
}

float getGyroRate() {
  Wire.beginTransmission(MPU);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 2, true);
  int16_t raw = (Wire.read() << 8) | Wire.read();
  return (raw / 131.0) - gyroOffset;
}

void calibrate() {
  Serial.println("[CAL] Keep THIGH still for 2 seconds...");
  float sum = 0;
  for (int i = 0; i < 200; i++) {
    Wire.beginTransmission(MPU);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 2, true);
    sum += ((Wire.read() << 8) | Wire.read()) / 131.0;
    delay(10);
  }
  gyroOffset      = sum / 200.0;
  thighAngle      = getAccelAngle();
  shinAngleInterp = thighAngle;  // Sensible default until slave connects
  lastIMUTime     = millis();
  Serial.printf("[CAL] Offset: %.4f | Start angle: %.2f\n", gyroOffset, thighAngle);
}

void updateThighAngle() {
  unsigned long now = millis();
  float dt = (now - lastIMUTime) / 1000.0;
  lastIMUTime = now;
  if (dt <= 0 || dt > 0.5) return;

  float gyro  = getGyroRate();
  float accel = getAccelAngle();
  if (abs(gyro) < 0.3) gyro = 0;  // Dead-zone

  thighAngle = CF_ALPHA * (thighAngle + gyro * dt)
             + (1.0 - CF_ALPHA) * accel;
}

// ── Slave TCP — non-blocking ──────────────────────────────────────────────────
void pollSlave() {
  if (!slaveClient || !slaveClient.connected()) {
    slaveClient = imuTcpServer.available();
    if (slaveClient) {
      slaveBuffer     = "";
      shinAngleInterp = shinAngle;
      Serial.println("[TCP] Shin IMU connected");
    }
    return;
  }

  while (slaveClient.available()) {
    char c = slaveClient.read();
    if (c == '\r') continue;
    if (c == '\n') {
      slaveBuffer.trim();
      if (slaveBuffer.length() > 0) {
        float val = slaveBuffer.toFloat();
        if (val >= -180.0 && val <= 180.0) {
          unsigned long now = millis();
          if (lastShinReceived > 0) {
            lastShinInterval = constrain(now - lastShinReceived, 5, 200);
          }
          lastShinReceived = now;
          shinAnglePrev    = shinAngleInterp;  // FROM current interpolated position
          shinAngle        = val;              // TO new target
        }
      }
      slaveBuffer = "";
    } else {
      if (slaveBuffer.length() < 20) slaveBuffer += c;
      else slaveBuffer = "";  // Overflow guard
    }
  }

  // Interpolate shin smoothly between received packets
  if (lastShinReceived > 0 && lastShinInterval > 0) {
    unsigned long elapsed = millis() - lastShinReceived;
    float alpha = constrain((float)elapsed / (float)lastShinInterval, 0.0f, 1.0f);
    shinAngleInterp = shinAnglePrev + alpha * (shinAngle - shinAnglePrev);
  }
}

// ── WebSocket event handler ───────────────────────────────────────────────────
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {

  if (type == WS_EVT_CONNECT) {
    client->text("Connected");
    Serial.printf("[WS] Client #%u connected\n", client->id());

  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("[WS] Client #%u disconnected\n", client->id());
    if (ws.count() == 0) {
      trackingActive = false;
      updateHardwareFeedback();
      Serial.println("[WS] No clients — auto stopped");
    }

  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len
        && info->opcode == WS_TEXT) {
      data[len] = 0;
      String msg = (char *)data;
      msg.trim();

      if (msg == "start") {
        trackingActive = true;
        Serial.println("[CMD] Start");
      } else if (msg == "stop") {
        trackingActive = false;
        Serial.println("[CMD] Stop");
      }
      updateHardwareFeedback();
    }
  }
}

void setup() {
  setCpuFrequencyMhz(240);
  Serial.begin(115200);

  // EMG pins
  pinMode(emg1Pin, INPUT);
  pinMode(emg2Pin, INPUT);

  // IMU
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0x00);  // Wake MPU6050
  Wire.endTransmission(true);
  delay(100);

  Wire.beginTransmission(MPU);
  if (Wire.endTransmission() == 0) {
    Serial.println("[IMU] Thigh MPU6050 found");
  } else {
    Serial.println("[IMU] ERROR: Thigh MPU6050 not found — check wiring");
  }

  calibrate();
  updateHardwareFeedback();

  // WiFi AP
  WiFi.softAP(ssid, password);
  WiFi.setSleep(false);
  Serial.printf("[BOOT] AP IP: %s\n", WiFi.softAPIP().toString().c_str());

  // TCP server for shin slave
  imuTcpServer.begin();
  Serial.printf("[BOOT] IMU TCP server on port %d\n", IMU_PORT);

  // WebSocket
  ws.onEvent(onWsEvent);
  webServer.addHandler(&ws);
  webServer.begin();
  Serial.println("[BOOT] Ready — connect browser to ws://192.168.4.1/ws");
}

void loop() {
  yield();
  unsigned long now = millis();

  // Always update thigh angle every loop for accurate dt integration
  updateThighAngle();

  // Always poll shin slave regardless of tracking state
  pollSlave();

  if (now - lastCleanupTime >= CLEANUP_INTERVAL_MS) {
    lastCleanupTime = now;
    ws.cleanupClients(2);
  }

  if (!trackingActive) return;

  // Heap guard — skip sample if memory critically low
  if (ESP.getFreeHeap() < 10000) {
    Serial.printf("[WARN] Low heap: %u — skipping\n", ESP.getFreeHeap());
    delay(5);
    return;
  }

  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = now;

    if (ws.count() > 0) {
      int emg1 = analogRead(emg1Pin);
      int emg2 = analogRead(emg2Pin);

      // Format: "emg1,emg2,thighAngle,shinAngle"
      // Frontend parses: v1=emg1, v2=emg2, i1=thigh, i2=shin
      // Knee flexion computed on frontend as |i1 - i2|
      char buf[64];
      snprintf(buf, sizeof(buf), "%d,%d,%.2f,%.2f",
               emg1, emg2, thighAngle, shinAngleInterp);
      ws.textAll(buf);

      Serial.printf("[OUT] EMG: %d/%d | Thigh: %.2f | Shin: %.2f | Knee: %.2f\n",
                    emg1, emg2, thighAngle, shinAngleInterp,
                    abs(thighAngle - shinAngleInterp));
    }
  }
}