#include <Arduino.h>
#include "pump.h"
#include "vacuum_pump.h"
#include "wifi_manager.h"
#include "web_server.h"

// with 6612FNG

// WiFi Configuration
const char* ssid = "ssid";        // Change to your WiFi name
const char* password = "pwd"; // Change to your WiFi password

// Global objects
PeristalticPump pump;
VacuumPump vacuumPump;
WiFiManager wifiManager(ssid, password);
WebServerManager webServer(&pump, &vacuumPump);



void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("=== ESP32-S3 Pump Controller (Peristaltic + Vacuum) ===");

  // Initialize pumps
  pump.begin();
  vacuumPump.begin();

  // Connect to WiFi
  wifiManager.connect();

  // Setup and start web server
  webServer.begin();
  
  if (wifiManager.isWiFiConnected()) {
    webServer.printServerInfo();
  } else {
    Serial.println("[Main] WiFi not connected. Web server may not be accessible.");
  }
  
  Serial.println("[Main] Setup complete. System ready.");
}

void loop() {
  // Handle Web Server Requests
  webServer.handleClient();
  
  // Update pumps (handles timed runs)
  pump.update();
  vacuumPump.update();
  
  // Can add other background tasks here
  delay(10);
}