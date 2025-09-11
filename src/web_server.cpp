#include "web_server.h"

WebServerManager::WebServerManager(PeristalticPump* pumpInstance, VacuumPump* vacuumPumpInstance) : server(80) {
  pump = pumpInstance;
  vacuumPump = vacuumPumpInstance;
}

void WebServerManager::begin() {
  // Setup Web Server Routes
  server.on("/", [this]() { handleRoot(); });
  server.on("/test", [this]() { handleTest(); });
  server.on("/api/control", HTTP_POST, [this]() { handleControl(); });
  server.on("/api/vacuum", HTTP_POST, [this]() { handleVacuumControl(); });
  server.on("/api/status", [this]() { handleStatus(); });
  
  // Start Web Server
  server.begin();
  Serial.println("[Web] Web server started");
}

void WebServerManager::handleClient() {
  server.handleClient();
}

void WebServerManager::printServerInfo() const {
  Serial.println("[Web] Visit http://" + WiFi.localIP().toString() + " to control the peristaltic pump");
}

String WebServerManager::generateHTML() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>Peristaltic Pump Controller</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 40px; background-color: #f0f0f0; }";
  html += ".container { background-color: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += ".control-group { margin: 20px 0; text-align: center; }";
  html += "button { padding: 15px 30px; margin: 10px; font-size: 16px; border: none; border-radius: 5px; cursor: pointer; }";
  html += ".forward { background-color: #4CAF50; color: white; }";
  html += ".reverse { background-color: #f44336; color: white; }";
  html += ".stop { background-color: #ff9800; color: white; }";
  html += "button:hover { opacity: 0.8; }";
  html += ".status { margin: 20px 0; padding: 15px; background-color: #e7f3ff; border-radius: 5px; }";
  html += "input[type='range'] { width: 200px; margin: 10px; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Peristaltic Pump Controller</h1>";
  
  // Display Current Status - Peristaltic Pump
  html += "<div class='status' id='pump-status'>";
  html += "<h3>Peristaltic Pump Status:</h3>";
  switch (pump->getCurrentState()) {
    case PUMP_STOPPED:
      html += "<p>Status: Stopped</p>";
      break;
    case PUMP_FORWARD:
      html += "<p>Status: Forward (Sample Intake)</p>";
      break;
    case PUMP_REVERSE:
      html += "<p>Status: Reverse (Liquid Extraction)</p>";
      break;
  }
  html += "<p>Speed: " + String(pump->getCurrentSpeed()) + "/255 (" + String((pump->getCurrentSpeed() * 100) / 255) + "%)</p>";
  if (pump->getIsTimedRun() && pump->getCurrentState() != PUMP_STOPPED) {
    uint32_t remainingTime = pump->getRemainingTime();
    html += "<p>Remaining time: " + String(remainingTime) + " seconds</p>";
  }
  html += "</div>";
  
  // Display Current Status - Vacuum Pump
  html += "<div class='status' id='vacuum-status'>";
  html += "<h3>Vacuum Pump Status:</h3>";
  switch (vacuumPump->getCurrentState()) {
    case VACUUM_STOPPED:
      html += "<p>Status: Stopped</p>";
      break;
    case VACUUM_RUNNING:
      html += "<p>Status: Running (Vacuum Generation)</p>";
      break;
  }
  html += "<p>Speed: " + String(vacuumPump->getCurrentSpeed()) + "/255 (" + String((vacuumPump->getCurrentSpeed() * 100) / 255) + "%)</p>";
  if (vacuumPump->getIsTimedRun() && vacuumPump->getCurrentState() != VACUUM_STOPPED) {
    uint32_t remainingTime = vacuumPump->getRemainingTime();
    html += "<p>Remaining time: " + String(remainingTime) + " seconds</p>";
  }
  html += "</div>";
  
  // Control Buttons - Peristaltic Pump
  html += "<div class='control-group'>";
  html += "<h3>Peristaltic Pump Control:</h3>";
  html += "<button class='forward' onclick=\"controlPump('forward')\">Forward (Sample Intake)</button>";
  html += "<button class='reverse' onclick=\"controlPump('reverse')\">Reverse (Liquid Extraction)</button>";
  html += "<button class='stop' onclick=\"controlPump('stop')\">Stop</button>";
  html += "</div>";
  
  // Control Buttons - Vacuum Pump
  html += "<div class='control-group'>";
  html += "<h3>Vacuum Pump Control:</h3>";
  html += "<button class='forward' onclick=\"controlVacuumPump('start')\">Start Vacuum</button>";
  html += "<button class='stop' onclick=\"controlVacuumPump('stop')\">Stop Vacuum</button>";
  html += "<button class='stop' onclick=\"controlVacuumPump('emergency')\" style='background-color: #ff0000;'>Emergency Stop</button>";
  html += "</div>";
  
  // Speed Control
  html += "<div class='control-group'>";
  html += "<h3>Speed Control:</h3>";
  html += "<input type='range' id='speedSlider' min='100' max='1023' value='" + String(pump->getCurrentSpeed()) + "' onchange=\"updateSpeed(this.value)\">";
  html += "<span id='speedValue'>" + String(pump->getCurrentSpeed()) + "</span>";
  html += "</div>";
  
  // Duration Control
  html += "<div class='control-group'>";
  html += "<h3>Run Duration (seconds):</h3>";
  html += "<input type='number' id='durationInput' min='1' max='300' value='" + String(pump->getRunDuration()) + "' style='width: 100px; padding: 5px; margin: 10px;'>";
  html += "<span> seconds</span>";
  html += "</div>";
  
  html += "</div>";
  
  // JavaScript
  html += "<script>";
  html += "function controlPump(action) {";
  html += "  console.log('controlPump called with action:', action);";
  html += "  var speed = parseInt(document.getElementById('speedSlider').value);";
  html += "  var duration = parseInt(document.getElementById('durationInput').value);";
  html += "  console.log('Sending:', {action: action, speed: speed, duration: duration});";
  html += "  fetch('/api/control', {";
  html += "    method: 'POST',";
  html += "    headers: { 'Content-Type': 'application/json' },";
  html += "    body: JSON.stringify({ action: action, speed: speed, duration: duration })";
  html += "  }).then(response => response.json())";
  html += "    .then(data => {";
  html += "      console.log('Response:', data);";
  html += "      alert('Pump control: ' + (data.success ? 'Success' : 'Failed'));";
  html += "    });";
  html += "}";
  html += "function controlVacuumPump(action) {";
  html += "  console.log('controlVacuumPump called with action:', action);";
  html += "  var speed = parseInt(document.getElementById('speedSlider').value);";
  html += "  var duration = parseInt(document.getElementById('durationInput').value);";
  html += "  console.log('Sending vacuum pump:', {action: action, speed: speed, duration: duration});";
  html += "  fetch('/api/vacuum', {";
  html += "    method: 'POST',";
  html += "    headers: { 'Content-Type': 'application/json' },";
  html += "    body: JSON.stringify({ action: action, speed: speed, duration: duration })";
  html += "  }).then(response => response.json())";
  html += "    .then(data => {";
  html += "      console.log('Response:', data);";
  html += "      alert('Vacuum control: ' + (data.success ? 'Success' : 'Failed'));";
  html += "    });";
  html += "}";
  html += "function updateSpeed(value) {";
  html += "  document.getElementById('speedValue').textContent = value;";
  html += "  console.log('Speed updated to:', value);";
  html += "}";
  html += "function updateStatus() {";
  html += "  fetch('/api/status')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      console.log('Status response:', data);";
  html += "    });";
  html += "}";
  html += "</script>";
  
  html += "</body></html>";
  
  return html;
}

void WebServerManager::handleRoot() {
  Serial.println("[Web] Handling root request");
  String html = generateHTML();
  Serial.println("[Web] Generated HTML length: " + String(html.length()));
  
  // Debug: Print first 500 characters of HTML
  String debugHtml = html.substring(0, min(500, (int)html.length()));
  Serial.println("[Web] HTML preview: " + debugHtml);
  
  server.send(200, "text/html", html);
}

void WebServerManager::handleTest() {
  String html = "<!DOCTYPE html><html><head><title>Test Page</title></head><body>";
  html += "<h1>Test Page</h1>";
  html += "<button onclick='testFunction()'>Test Button</button>";
  html += "<p id='result'>Click the button to test</p>";
  html += "<hr>";
  html += "<h2>Main Page Test</h2>";
  html += "<button onclick='controlPump(\"forward\")'>Test Forward</button>";
  html += "<button onclick='controlVacuumPump(\"start\")'>Test Vacuum Start</button>";
  html += "<input type='range' id='speedSlider' min='50' max='255' value='100' onchange='updateSpeed(this.value)'>";
  html += "<span id='speedValue'>100</span>";
  html += "<script>";
  html += "function testFunction() {";
  html += "  document.getElementById('result').innerHTML = 'Button clicked! JavaScript is working.';";
  html += "}";
  html += "function controlPump(action) {";
  html += "  console.log('controlPump called with action:', action);";
  html += "  var speed = parseInt(document.getElementById('speedSlider').value);";
  html += "  var duration = 5;";
  html += "  console.log('Sending:', {action: action, speed: speed, duration: duration});";
  html += "  fetch('/api/control', {";
  html += "    method: 'POST',";
  html += "    headers: { 'Content-Type': 'application/json' },";
  html += "    body: JSON.stringify({ action: action, speed: speed, duration: duration })";
  html += "  }).then(response => response.json())";
  html += "    .then(data => {";
  html += "      console.log('Response:', data);";
  html += "      alert('Pump control: ' + (data.success ? 'Success' : 'Failed'));";
  html += "    });";
  html += "}";
  html += "function controlVacuumPump(action) {";
  html += "  console.log('controlVacuumPump called with action:', action);";
  html += "  var speed = parseInt(document.getElementById('speedSlider').value);";
  html += "  var duration = 5;";
  html += "  console.log('Sending vacuum pump:', {action: action, speed: speed, duration: duration});";
  html += "  fetch('/api/vacuum', {";
  html += "    method: 'POST',";
  html += "    headers: { 'Content-Type': 'application/json' },";
  html += "    body: JSON.stringify({ action: action, speed: speed, duration: duration })";
  html += "  }).then(response => response.json())";
  html += "    .then(data => {";
  html += "      console.log('Response:', data);";
  html += "      alert('Vacuum control: ' + (data.success ? 'Success' : 'Failed'));";
  html += "    });";
  html += "}";
  html += "function updateSpeed(value) {";
  html += "  document.getElementById('speedValue').textContent = value;";
  html += "  console.log('Speed updated to:', value);";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

String WebServerManager::parseAction(const String& body) {
  if (body.indexOf("\"action\":\"forward\"") >= 0) {
    return "forward";
  } else if (body.indexOf("\"action\":\"reverse\"") >= 0) {
    return "reverse";
  } else if (body.indexOf("\"action\":\"stop\"") >= 0) {
    return "stop";
  }
  return "";
}

String WebServerManager::parseVacuumAction(const String& body) {
  if (body.indexOf("\"action\":\"start\"") >= 0) {
    return "start";
  } else if (body.indexOf("\"action\":\"stop\"") >= 0) {
    return "stop";
  } else if (body.indexOf("\"action\":\"emergency\"") >= 0) {
    return "emergency";
  }
  return "";
}

uint16_t WebServerManager::parseSpeed(const String& body) {
  int speedStart = body.indexOf("\"speed\":");
  if (speedStart >= 0) {
    speedStart += 8; // Skip "speed":
    int speedEnd = body.indexOf(",", speedStart);
    if (speedEnd < 0) speedEnd = body.indexOf("}", speedStart);
    if (speedEnd > speedStart) {
      String speedStr = body.substring(speedStart, speedEnd);
      uint16_t speed = speedStr.toInt();
      if (speed < 100) speed = 100;  // Minimum 10% of 1023
      if (speed > 1023) speed = 1023;  // Maximum 100% of 1023
      Serial.println("[Web] Speed extracted: '" + speedStr + "' -> " + String(speed));
      return speed;
    }
  }
  return pump->getCurrentSpeed();
}

uint8_t WebServerManager::parseVacuumSpeed(const String& body) {
  int speedStart = body.indexOf("\"speed\":");
  if (speedStart >= 0) {
    speedStart += 8; // Skip "speed":
    int speedEnd = body.indexOf(",", speedStart);
    if (speedEnd < 0) speedEnd = body.indexOf("}", speedStart);
    if (speedEnd > speedStart) {
      String speedStr = body.substring(speedStart, speedEnd);
      uint16_t rawSpeed = speedStr.toInt();
      // Convert from 0-1023 range to 0-100 percentage
      uint8_t speedPercent = (rawSpeed * 100) / 1023;
      if (speedPercent < 10) speedPercent = 10;  // Minimum 10%
      if (speedPercent > 100) speedPercent = 100;  // Maximum 100%
      Serial.println("[Web] Vacuum Speed extracted: '" + speedStr + "' -> " + String(speedPercent) + "%");
      return speedPercent;
    }
  }
  return vacuumPump->getCurrentSpeed();
}

uint32_t WebServerManager::parseDuration(const String& body) {
  int durationStart = body.indexOf("\"duration\":");
  if (durationStart >= 0) {
    durationStart += 11; // Skip "duration":
    int durationEnd = body.indexOf(",", durationStart);
    if (durationEnd < 0) durationEnd = body.indexOf("}", durationStart);
    if (durationEnd > durationStart) {
      String durationStr = body.substring(durationStart, durationEnd);
      uint32_t duration = durationStr.toInt();
      if (duration < 1) duration = 1;
      if (duration > 300) duration = 300;
      Serial.println("[Web] Duration extracted: '" + durationStr + "' -> " + String(duration));
      return duration;
    }
  }
  return pump->getRunDuration();
}

void WebServerManager::handleControl() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"success\": false, \"message\": \"Method not allowed\"}");
    return;
  }
  
  String body = server.arg("plain");
  
  // Debug: Print received JSON
  Serial.println("[Web] Received JSON: " + body);
  Serial.println("[Web] JSON length: " + String(body.length()));
  
  // Parse JSON
  String action = parseAction(body);
  uint16_t speed = parseSpeed(body);
  uint32_t duration = parseDuration(body);
  
  // Debug: Print parsed values
  Serial.println("[Web] Parsed - Action: " + action + ", Speed: " + String(speed) + ", Duration: " + String(duration));
  
  // Execute control operation
  if (action == "forward") {
    pump->controlPump(PUMP_FORWARD, speed, duration);
    String message = "Forward started";
    if (duration > 0) {
      message += " for " + String(duration) + " seconds";
    }
    server.send(200, "application/json", "{\"success\": true, \"message\": \"" + message + "\"}");
  } else if (action == "reverse") {
    pump->controlPump(PUMP_REVERSE, speed, duration);
    String message = "Reverse started";
    if (duration > 0) {
      message += " for " + String(duration) + " seconds";
    }
    server.send(200, "application/json", "{\"success\": true, \"message\": \"" + message + "\"}");
  } else if (action == "stop") {
    pump->controlPump(PUMP_STOPPED, speed, 0);
    server.send(200, "application/json", "{\"success\": true, \"message\": \"Stopped\"}");
  } else {
    server.send(400, "application/json", "{\"success\": false, \"message\": \"Invalid operation\"}");
  }
}

String WebServerManager::generateStatusJSON() {
  String json = "{";
  json += "\"success\": true,";
  
  // Peristaltic pump status
  json += "\"pump\": {";
  json += "\"state\": ";
  switch (pump->getCurrentState()) {
    case PUMP_STOPPED:
      json += "\"stopped\"";
      break;
    case PUMP_FORWARD:
      json += "\"forward\"";
      break;
    case PUMP_REVERSE:
      json += "\"reverse\"";
      break;
  }
  json += ",\"speed\": " + String(pump->getCurrentSpeed());
  json += ",\"speedPercent\": " + String((pump->getCurrentSpeed() * 100) / 255);
  
  // Add remaining time if running on timer
  if (pump->getIsTimedRun() && pump->getCurrentState() != PUMP_STOPPED) {
    uint32_t remainingTime = pump->getRemainingTime();
    json += ",\"remainingTime\": " + String(remainingTime);
    json += ",\"isTimedRun\": true";
  } else {
    json += ",\"remainingTime\": 0";
    json += ",\"isTimedRun\": false";
  }
  json += "},";
  
  // Vacuum pump status
  json += "\"vacuum\": {";
  json += "\"state\": ";
  switch (vacuumPump->getCurrentState()) {
    case VACUUM_STOPPED:
      json += "\"stopped\"";
      break;
    case VACUUM_RUNNING:
      json += "\"running\"";
      break;
  }
  json += ",\"speed\": " + String(vacuumPump->getCurrentSpeed());
  json += ",\"speedPercent\": " + String((vacuumPump->getCurrentSpeed() * 100) / 255);
  
  // Add remaining time if running on timer
  if (vacuumPump->getIsTimedRun() && vacuumPump->getCurrentState() != VACUUM_STOPPED) {
    uint32_t remainingTime = vacuumPump->getRemainingTime();
    json += ",\"remainingTime\": " + String(remainingTime);
    json += ",\"isTimedRun\": true";
  } else {
    json += ",\"remainingTime\": 0";
    json += ",\"isTimedRun\": false";
  }
  json += "}";
  
  json += "}";
  
  return json;
}

void WebServerManager::handleStatus() {
  server.send(200, "application/json", generateStatusJSON());
}

void WebServerManager::handleVacuumControl() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"success\": false, \"message\": \"Method not allowed\"}");
    return;
  }
  
  String body = server.arg("plain");
  
  // Debug: Print received JSON
  Serial.println("[Web] Received vacuum JSON: " + body);
  
  // Parse JSON
  String action = parseVacuumAction(body);
  uint8_t speed = parseVacuumSpeed(body);
  uint32_t duration = parseDuration(body);
  
  // Debug: Print parsed values
  Serial.println("[Web] Vacuum Parsed - Action: " + action + ", Speed: " + String(speed) + ", Duration: " + String(duration));
  
  // Execute vacuum pump control operation
  if (action == "start") {
    vacuumPump->controlVacuumPump(VACUUM_RUNNING, speed, duration);
    String message = "Vacuum pump started";
    if (duration > 0) {
      message += " for " + String(duration) + " seconds";
    }
    server.send(200, "application/json", "{\"success\": true, \"message\": \"" + message + "\"}");
  } else if (action == "stop") {
    vacuumPump->controlVacuumPump(VACUUM_STOPPED, speed, 0);
    server.send(200, "application/json", "{\"success\": true, \"message\": \"Vacuum pump stopped\"}");
  } else if (action == "emergency") {
    vacuumPump->emergencyStop();
    server.send(200, "application/json", "{\"success\": true, \"message\": \"Emergency stop activated\"}");
  } else {
    server.send(400, "application/json", "{\"success\": false, \"message\": \"Invalid vacuum pump operation\"}");
  }
}
