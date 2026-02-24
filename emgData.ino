#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

const char* ssid = "UTBiome_Knee";
const char* password = "password123";
const int sensorPin = 3;
bool trackingActive = false;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // Speed fix: WebSocket

void updateHardwareFeedback() {
  if (trackingActive) {
    neopixelWrite(RGB_BUILTIN, 0, 64, 0); // Green
  } else {
    neopixelWrite(RGB_BUILTIN, 64, 0, 0); // Red
  }
}

void setup() {
  // HEAT FIX: Lower CPU frequency to 80MHz (Standard is 240MHz)
  setCpuFrequencyMhz(80); 
  
  Serial.begin(115200);
  updateHardwareFeedback();

  WiFi.softAP(ssid, password);
  
  // HEAT FIX: Power Management for Wi-Fi
  WiFi.setSleep(true); 

  // WebSocket event handler
  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) client->text("Connected");
  });
  server.addHandler(&ws);

  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("state")) {
      String state = request->getParam("state")->value();
      trackingActive = (state == "start");
      updateHardwareFeedback();
      request->send(200, "text/plain", "OK");
    }
  });

  server.begin();
}

void loop() {
  ws.cleanupClients();
  
  if (trackingActive) {
    // Read and send immediately
    int val = analogRead(sensorPin);
    ws.textAll(String(val));
    delay(20); // ~50Hz sampling is usually enough for smooth visual EMG
  } else {
    // HEAT FIX: Slow down the loop when idle
    delay(100); 
  }
}