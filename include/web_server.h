#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include "pump.h"
#include "vacuum_pump.h"

class WebServerManager {
private:
  WebServer server;
  PeristalticPump* pump;
  VacuumPump* vacuumPump;
  
  // Web page generation
  String generateHTML();
  String generateStatusJSON();
  
  // Request handlers
  void handleRoot();
  void handleTest();
  void handleControl();
  void handleVacuumControl();
  void handleStatus();
  
  // JSON parsing helpers
  String parseAction(const String& body);
  String parseVacuumAction(const String& body);
  uint16_t parseSpeed(const String& body);
  uint8_t parseVacuumSpeed(const String& body);
  uint32_t parseDuration(const String& body);
  
public:
  WebServerManager(PeristalticPump* pumpInstance, VacuumPump* vacuumPumpInstance);
  void begin();
  void handleClient();
  void printServerInfo() const;
};

#endif // WEB_SERVER_H
