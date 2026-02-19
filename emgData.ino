#include <WiFi.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "UTBiome_Knee";
const char* password = "password123";

const int sensorPin = 3;

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  // Start WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.softAPIP());

  // Allow browser access (CORS fix)
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  // Data endpoint
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    int sensorValue = analogRead(sensorPin);
    request->send(200, "text/plain", String(sensorValue));
  });

  server.begin();
}

void loop() {}
