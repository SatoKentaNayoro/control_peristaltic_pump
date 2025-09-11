#include "wifi_manager.h"

WiFiManager::WiFiManager(const char* wifi_ssid, const char* wifi_password) {
  ssid = wifi_ssid;
  password = wifi_password;
  connectionAttempts = 20;
  isConnected = false;
}

bool WiFiManager::connect() {
  Serial.print("[WiFi] Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < connectionAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    isConnected = true;
    Serial.println("[WiFi] WiFi connected successfully!");
    Serial.print("[WiFi] IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    isConnected = false;
    Serial.println("[WiFi] WiFi connection failed!");
    Serial.println("[WiFi] Please check your WiFi credentials and try again.");
    return false;
  }
}

String WiFiManager::getLocalIP() const {
  if (isConnected) {
    return WiFi.localIP().toString();
  }
  return "Not connected";
}

void WiFiManager::disconnect() {
  WiFi.disconnect();
  isConnected = false;
  Serial.println("[WiFi] WiFi disconnected");
}

void WiFiManager::printStatus() const {
  if (isConnected) {
    Serial.println("[WiFi] Status: Connected");
    Serial.print("[WiFi] IP: ");
    Serial.println(getLocalIP());
  } else {
    Serial.println("[WiFi] Status: Disconnected");
  }
}
