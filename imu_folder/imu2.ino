#include <WiFi.h>
#include <Wire.h>

const char* ssid      = "UTBiome_Knee";
const char* password  = "password123";
const char* MASTER_IP = "192.168.4.1";
const int   MASTER_PORT = 9000;

#define SDA_PIN 4
#define SCL_PIN 5
const int MPU = 0x68;

const float CF_ALPHA  = 0.98;
float shinAngle       = 0.0;
float gyroOffset      = 0.0;
unsigned long lastIMUTime     = 0;
unsigned long lastSendTime    = 0;
unsigned long lastConnectTime = 0;
const unsigned long SEND_INTERVAL_MS   = 20;
const unsigned long RECONNECT_INTERVAL = 3000;

WiFiClient client;

float getAccelAngle() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);
  int16_t ax = (Wire.read() << 8) | Wire.read();
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
  Serial.println("[CAL] Keep SHIN still for 2 seconds...");
  float sum = 0;
  for (int i = 0; i < 200; i++) {
    Wire.beginTransmission(MPU);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 2, true);
    sum += ((Wire.read() << 8) | Wire.read()) / 131.0;
    delay(10);
  }
  gyroOffset = sum / 200.0;
  shinAngle  = getAccelAngle();
  lastIMUTime = millis();
  Serial.printf("[CAL] Offset: %.4f | Start angle: %.2f\n", gyroOffset, shinAngle);
}

void updateShinAngle() {
  unsigned long now = millis();
  float dt = (now - lastIMUTime) / 1000.0;
  lastIMUTime = now;
  if (dt <= 0 || dt > 0.5) return;

  float gyro  = getGyroRate();
  float accel = getAccelAngle();
  if (abs(gyro) < 0.3) gyro = 0;

  shinAngle = CF_ALPHA * (shinAngle + gyro * dt)
            + (1.0 - CF_ALPHA) * accel;
}

void connectToMaster() {
  if (millis() - lastConnectTime < RECONNECT_INTERVAL) return;
  lastConnectTime = millis();
  Serial.println("[TCP] Connecting to master...");
  if (client.connect(MASTER_IP, MASTER_PORT, 5000)) {  // 5s timeout
    Serial.println("[TCP] Connected!");
  } else {
    Serial.println("[TCP] Failed — retrying in 3s");
  }
}

void setup() {
  setCpuFrequencyMhz(240);
  Serial.begin(115200);
  delay(3000);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);
  delay(100);

  Wire.beginTransmission(MPU);
  if (Wire.endTransmission() == 0) {
    Serial.println("[IMU] Shin MPU6050 found");
  } else {
    Serial.println("[IMU] ERROR: Shin MPU6050 not found — check SDA/SCL and AD0 pin");
  }

  calibrate();

  WiFi.begin(ssid, password);
  Serial.print("[WiFi] Connecting");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected — IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] Failed to connect — restarting");
    ESP.restart();
  }

  delay(2000);  // Wait for master TCP server to be ready
  connectToMaster();
}

void loop() {
  updateShinAngle();

  if (!client || !client.connected()) {
    connectToMaster();
    delay(10);
    return;
  }

  unsigned long now = millis();
  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = now;

    char buf[20];
    snprintf(buf, sizeof(buf), "%.2f\r\n", shinAngle);

    if (client.print(buf) == 0) {
      Serial.println("[TCP] Send failed — reconnecting");
      client.stop();
    } else {
      Serial.printf("[SHIN] %.2f deg\n", shinAngle);
    }
  }

  yield();
}