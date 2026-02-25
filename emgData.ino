#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#define RGB_BUILTIN 21

const char* ssid = "UTBiome_Knee";
const char* password = "password123";

// Updated Sensor Pins
const int emg1Pin = 2; 
const int emg2Pin = 3; 

bool trackingActive = false;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void updateHardwareFeedback() {
  if (trackingActive) {
    neopixelWrite(RGB_BUILTIN, 0, 64, 0); // Green
  } else {
    neopixelWrite(RGB_BUILTIN, 64, 0, 0); // Red
  }
}

void setup() {
  setCpuFrequencyMhz(80); 
  Serial.begin(115200);
  
  // Initialize Pins
  pinMode(emg1Pin, INPUT);
  pinMode(emg2Pin, INPUT);

  updateHardwareFeedback();

  WiFi.softAP(ssid, password);
  WiFi.setSleep(true); 

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      client->text("Connected");
    } else if (type == WS_EVT_DATA) {
      String msg = "";
      for (size_t i = 0; i < len; i++) msg += (char)data[i];

      if (msg == "start") {
        trackingActive = true;
      } else if (msg == "stop") {
        trackingActive = false;
      }
      updateHardwareFeedback();
    }
  });
  
  server.addHandler(&ws);
  server.begin();
}

void loop() {
  ws.cleanupClients();
  
  if (trackingActive) {
    // Read both sensors
    int val1 = analogRead(emg1Pin);
    int val2 = analogRead(emg2Pin);

    // Format as "val1,val2" for easy parsing on the frontend
    String combinedData = String(val1) + "," + String(val2);
    
    ws.textAll(combinedData);
    
    // 50Hz sampling for smooth visual data
    delay(20); 
  } else {
    delay(100); 
  }
}