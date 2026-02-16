#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <math.h>

// --- Configuration ---
const char* ssid = "EMG_Test_Bench";
const char* password = "password123";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

unsigned long lastTime = 0;
const int timerDelay = 20; // Sends data every 20ms (50Hz)

// --- WebSocket Event Handler ---
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Browser connected from IP: %s\n", client->remoteIP().toString().c_str());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Browser disconnected");
  }
}

void setup() {
  Serial.begin(115200);

  // 1. Start Access Point
  WiFi.softAP(ssid, password);
  Serial.println("\n--- ESP32 SIMULATOR STARTING ---");
  Serial.print("Connect to WiFi: "); Serial.println(ssid);
  Serial.print("IP Address: ");      Serial.println(WiFi.softAPIP());

  // 2. Setup WebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // 3. Start Web Server
  server.begin();
}

void loop() {
  ws.cleanupClients(); // Clean up "ghost" connections

  if (millis() - lastTime > timerDelay) {
    // SIMULATION LOGIC:
    // We create a sine wave to act as a placeholder for the EMG sensor
    float angle = millis() / 250.0; 
    int simulatedValue = 2048 + (sin(angle) * 1000); // Oscillates 1048 to 3048

    // Create JSON Packet
    StaticJsonDocument<64> doc;
    doc["v"] = simulatedValue;
    doc["t"] = millis();

    String jsonString;
    serializeJson(doc, jsonString);
    
    // Push data to all connected browsers
    ws.textAll(jsonString);
    
    lastTime = millis();
  }
}