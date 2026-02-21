#include <WiFi.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "UTBiome_Knee";
const char* password = "password123";

const int sensorPin = 3;
bool trackingActive = false;

AsyncWebServer server(80);

// Helper function to set the onboard RGB LED color
void updateHardwareFeedback() {
  if (trackingActive) {
    // Green: (Pin, Red, Green, Blue)
    neopixelWrite(RGB_BUILTIN, 0, 64, 0); 
    Serial.println("LED: Green (Tracking)");
  } else {
    // Red: (Pin, Red, Green, Blue)
    neopixelWrite(RGB_BUILTIN, 64, 0, 0); 
    Serial.println("LED: Red (Idle)");
  }
}

void setup() {
  Serial.begin(115200);
  
  // No pinMode needed for neopixelWrite, it handles it internally
  
  // Initialize to Red (System Idle)
  updateHardwareFeedback();

  WiFi.softAP(ssid, password);
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.softAPIP());

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    if (trackingActive) {
      int sensorValue = analogRead(sensorPin);
      request->send(200, "text/plain", String(sensorValue));
    } else {
      request->send(200, "text/plain", "OFF");
    }
  });

  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("state")) {
      String state = request->getParam("state")->value();
      trackingActive = (state == "start");
      updateHardwareFeedback(); 
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Missing State");
    }
  });

  server.begin();
}

void loop() {
  // The web server runs in the background
}