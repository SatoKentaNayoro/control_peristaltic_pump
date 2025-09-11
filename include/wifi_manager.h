#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

class WiFiManager {
private:
  const char* ssid;
  const char* password;
  int connectionAttempts;
  bool isConnected;
  
public:
  WiFiManager(const char* wifi_ssid, const char* wifi_password);
  bool connect();
  bool isWiFiConnected() const { return isConnected; }
  String getLocalIP() const;
  void disconnect();
  void printStatus() const;
};

#endif // WIFI_MANAGER_H
