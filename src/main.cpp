#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// with 6612FNG

// WiFi Configuration
const char* ssid = "ssid";        // Change to your WiFi name
const char* password = "pwd"; // Change to your WiFi password

// Web Server
WebServer server(80);

// Peristaltic Pump States
enum PumpState {
  PUMP_STOPPED,
  PUMP_FORWARD,  // Forward rotation - Sample intake
  PUMP_REVERSE   // Reverse rotation - Liquid extraction
};

PumpState currentPumpState = PUMP_STOPPED;
uint8_t currentSpeed = 100; // Default speed
uint32_t runDuration = 5; // Default run duration in seconds
unsigned long pumpStartTime = 0; // When pump started running
bool isTimedRun = false; // Whether pump is running on timer

const int PIN_PWMA = 13;  // PWM-capable
const int PIN_STBY = 10;
const int PIN_AIN1 = 11;
const int PIN_AIN2 = 12;

// PWM
const int PWM_CH = 0;
const int PWM_FREQ = 20000;
const int PWM_RES = 8;

uint32_t g_lastDuty = 0;

void logPinStates(const char* prefix) {
  int ain1 = digitalRead(PIN_AIN1);
  int ain2 = digitalRead(PIN_AIN2);
  int stby = digitalRead(PIN_STBY);

  Serial.print(prefix);
  Serial.print(" AIN1=");
  Serial.print(ain1);
  Serial.print(" AIN2=");
  Serial.print(ain2);
  Serial.print(" STBY=");
  Serial.print(stby);
  Serial.print(" PWM(duty)=");
  Serial.print(g_lastDuty);
  Serial.print("/");
  Serial.print((1 << PWM_RES) - 1);
  Serial.print(" (");
  Serial.print((g_lastDuty * 100) / ((1 << PWM_RES) - 1));
  Serial.println("%)");
}

void motorCoast() {
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  ledcWrite(PWM_CH, 0);
  g_lastDuty = 0;

  Serial.println("[Motor] Coast (freewheel)");
  logPinStates("        ");
}

void motorBrake() {
  digitalWrite(PIN_AIN1, HIGH);
  digitalWrite(PIN_AIN2, HIGH);
  ledcWrite(PWM_CH, 0);
  g_lastDuty = 0;

  Serial.println("[Motor] Brake (short brake)");
  logPinStates("        ");
}

void motorForward(uint8_t speed) {
  digitalWrite(PIN_AIN1, HIGH);
  digitalWrite(PIN_AIN2, LOW);
  ledcWrite(PWM_CH, speed);
  g_lastDuty = speed;

  Serial.print("[Motor] Forward | speed=");
  Serial.print(speed);
  Serial.print(" (");
  Serial.print((speed * 100) / ((1 << PWM_RES) - 1));
  Serial.println("%)");
  logPinStates("        ");
}

void motorReverse(uint8_t speed) {
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, HIGH);
  ledcWrite(PWM_CH, speed);
  g_lastDuty = speed;

  Serial.print("[Motor] Reverse | speed=");
  Serial.print(speed);
  Serial.print(" (");
  Serial.print((speed * 100) / ((1 << PWM_RES) - 1));
  Serial.println("%)");
  logPinStates("        ");
}

// WiFi Connection Function
void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed!");
    Serial.println("Please check your WiFi credentials and try again.");
    // Continue anyway for testing
  }
}

// Control Peristaltic Pump Function
void controlPump(PumpState state, uint8_t speed = 100, uint32_t duration = 0) {
  currentPumpState = state;
  currentSpeed = speed;
  runDuration = duration;
  
  switch (state) {
    case PUMP_STOPPED:
      motorCoast();
      isTimedRun = false;
      break;
    case PUMP_FORWARD:
      motorForward(speed);
      if (duration > 0) {
        isTimedRun = true;
        pumpStartTime = millis();
      } else {
        isTimedRun = false;
      }
      break;
    case PUMP_REVERSE:
      motorReverse(speed);
      if (duration > 0) {
        isTimedRun = true;
        pumpStartTime = millis();
      } else {
        isTimedRun = false;
      }
      break;
  }
}

// Handle Root Path - Display Control Interface
void handleRoot() {
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
  
  // Display Current Status
  html += "<div class='status'>";
  html += "<h3>Current Status:</h3>";
  switch (currentPumpState) {
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
  html += "<p>Speed: " + String(currentSpeed) + "/255 (" + String((currentSpeed * 100) / 255) + "%)</p>";
  if (isTimedRun && currentPumpState != PUMP_STOPPED) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - pumpStartTime) / 1000;
    unsigned long remainingTime = (runDuration > elapsedTime) ? (runDuration - elapsedTime) : 0;
    html += "<p>Remaining time: " + String(remainingTime) + " seconds</p>";
  }
  html += "</div>";
  
  // Control Buttons
  html += "<div class='control-group'>";
  html += "<h3>Control Operations:</h3>";
  html += "<button class='forward' onclick='controlPump(\"forward\")'>Forward (Sample Intake)</button>";
  html += "<button class='reverse' onclick='controlPump(\"reverse\")'>Reverse (Liquid Extraction)</button>";
  html += "<button class='stop' onclick='controlPump(\"stop\")'>Stop</button>";
  html += "</div>";
  
  // Speed Control
  html += "<div class='control-group'>";
  html += "<h3>Speed Control:</h3>";
  html += "<input type='range' id='speedSlider' min='50' max='255' value='" + String(currentSpeed) + "' onchange='updateSpeed(this.value)'>";
  html += "<span id='speedValue'>" + String(currentSpeed) + "</span>";
  html += "</div>";
  
  // Duration Control
  html += "<div class='control-group'>";
  html += "<h3>Run Duration (seconds):</h3>";
  html += "<input type='number' id='durationInput' min='1' max='300' value='" + String(runDuration) + "' style='width: 100px; padding: 5px; margin: 10px;'>";
  html += "<span> seconds</span>";
  html += "</div>";
  
  html += "</div>";
  
  // JavaScript
  html += "<script>";
  html += "function controlPump(action) {";
  html += "  var speed = parseInt(document.getElementById('speedSlider').value);";
  html += "  var duration = parseInt(document.getElementById('durationInput').value);";
  html += "  console.log('Sending:', {action: action, speed: speed, duration: duration});";
  html += "  fetch('/api/control', {";
  html += "    method: 'POST',";
  html += "    headers: { 'Content-Type': 'application/json' },";
  html += "    body: JSON.stringify({ action: action, speed: speed, duration: duration })";
  html += "  }).then(response => response.json())";
  html += "    .then(data => {";
  html += "      if (data.success) {";
  html += "        updateStatus();";
  html += "        showMessage(data.message, 'success');";
  html += "      } else {";
  html += "        showMessage('Operation failed: ' + data.message, 'error');";
  html += "      }";
  html += "    });";
  html += "}";
  html += "function updateSpeed(value) {";
  html += "  document.getElementById('speedValue').textContent = value;";
  html += "}";
  html += "function updateStatus() {";
  html += "  fetch('/api/status')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      if (data.success) {";
  html += "        updateStatusDisplay(data);";
  html += "      }";
  html += "    });";
  html += "}";
  html += "function updateStatusDisplay(data) {";
  html += "  var statusText = '';";
  html += "  switch(data.state) {";
  html += "    case 'stopped': statusText = 'Stopped'; break;";
  html += "    case 'forward': statusText = 'Forward (Sample Intake)'; break;";
  html += "    case 'reverse': statusText = 'Reverse (Liquid Extraction)'; break;";
  html += "  }";
  html += "  document.querySelector('.status p:first-child').innerHTML = 'Status: ' + statusText;";
  html += "  document.querySelector('.status p:nth-child(2)').innerHTML = 'Speed: ' + data.speed + '/255 (' + data.speedPercent + '%)';";
  html += "  var statusDiv = document.querySelector('.status');";
  html += "  var remainingTimeP = statusDiv.querySelector('p:nth-child(3)');";
  html += "  if (remainingTimeP) {";
  html += "    remainingTimeP.remove();";
  html += "  }";
  html += "  if (data.isTimedRun && data.remainingTime > 0) {";
  html += "    var timeP = document.createElement('p');";
  html += "    timeP.innerHTML = 'Remaining time: ' + data.remainingTime + ' seconds';";
  html += "    statusDiv.appendChild(timeP);";
  html += "  }";
  html += "}";
  html += "function showMessage(message, type) {";
  html += "  var messageDiv = document.getElementById('messageDiv');";
  html += "  if (!messageDiv) {";
  html += "    messageDiv = document.createElement('div');";
  html += "    messageDiv.id = 'messageDiv';";
  html += "    messageDiv.style.cssText = 'position: fixed; top: 20px; right: 20px; padding: 10px 20px; border-radius: 5px; z-index: 1000;';";
  html += "    document.body.appendChild(messageDiv);";
  html += "  }";
  html += "  messageDiv.textContent = message;";
  html += "  messageDiv.style.backgroundColor = type === 'success' ? '#4CAF50' : '#f44336';";
  html += "  messageDiv.style.color = 'white';";
  html += "  setTimeout(() => { messageDiv.remove(); }, 3000);";
  html += "}";
  html += "// Auto-refresh status every 2 seconds";
  html += "setInterval(updateStatus, 2000);";
  html += "</script>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Handle API Control Requests
void handleControl() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"success\": false, \"message\": \"Method not allowed\"}");
    return;
  }
  
  String body = server.arg("plain");
  
  // Debug: Print received JSON
  Serial.println("Received JSON: " + body);
  
  // Simple JSON parsing
  String action = "";
  uint8_t speed = currentSpeed;
  uint32_t duration = runDuration;
  
  if (body.indexOf("\"action\":\"forward\"") >= 0) {
    action = "forward";
  } else if (body.indexOf("\"action\":\"reverse\"") >= 0) {
    action = "reverse";
  } else if (body.indexOf("\"action\":\"stop\"") >= 0) {
    action = "stop";
  }
  
  // Extract speed value - improved parsing
  int speedStart = body.indexOf("\"speed\":");
  if (speedStart >= 0) {
    speedStart += 8; // Skip "speed":
    int speedEnd = body.indexOf(",", speedStart);
    if (speedEnd < 0) speedEnd = body.indexOf("}", speedStart);
    if (speedEnd > speedStart) {
      String speedStr = body.substring(speedStart, speedEnd);
      speed = speedStr.toInt();
      if (speed < 50) speed = 50;
      if (speed > 255) speed = 255;
      Serial.println("Speed extracted: '" + speedStr + "' -> " + String(speed));
    }
  }
  
  // Extract duration value - improved parsing
  int durationStart = body.indexOf("\"duration\":");
  if (durationStart >= 0) {
    durationStart += 11; // Skip "duration":
    int durationEnd = body.indexOf(",", durationStart);
    if (durationEnd < 0) durationEnd = body.indexOf("}", durationStart);
    if (durationEnd > durationStart) {
      String durationStr = body.substring(durationStart, durationEnd);
      duration = durationStr.toInt();
      if (duration < 1) duration = 1;
      if (duration > 300) duration = 300;
      Serial.println("Duration extracted: '" + durationStr + "' -> " + String(duration));
    }
  }
  
  // Debug: Print parsed values
  Serial.println("Parsed - Action: " + action + ", Speed: " + String(speed) + ", Duration: " + String(duration));
  
  // Execute control operation
  if (action == "forward") {
    controlPump(PUMP_FORWARD, speed, duration);
    String message = "Forward started";
    if (duration > 0) {
      message += " for " + String(duration) + " seconds";
    }
    server.send(200, "application/json", "{\"success\": true, \"message\": \"" + message + "\"}");
  } else if (action == "reverse") {
    controlPump(PUMP_REVERSE, speed, duration);
    String message = "Reverse started";
    if (duration > 0) {
      message += " for " + String(duration) + " seconds";
    }
    server.send(200, "application/json", "{\"success\": true, \"message\": \"" + message + "\"}");
  } else if (action == "stop") {
    controlPump(PUMP_STOPPED, speed, 0);
    server.send(200, "application/json", "{\"success\": true, \"message\": \"Stopped\"}");
  } else {
    server.send(400, "application/json", "{\"success\": false, \"message\": \"Invalid operation\"}");
  }
}

// Handle Status Query
void handleStatus() {
  String json = "{";
  json += "\"success\": true,";
  json += "\"state\": ";
  switch (currentPumpState) {
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
  json += ",\"speed\": " + String(currentSpeed);
  json += ",\"speedPercent\": " + String((currentSpeed * 100) / 255);
  
  // Add remaining time if running on timer
  if (isTimedRun && currentPumpState != PUMP_STOPPED) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - pumpStartTime) / 1000;
    unsigned long remainingTime = (runDuration > elapsedTime) ? (runDuration - elapsedTime) : 0;
    json += ",\"remainingTime\": " + String(remainingTime);
    json += ",\"isTimedRun\": true";
  } else {
    json += ",\"remainingTime\": 0";
    json += ",\"isTimedRun\": false";
  }
  
  json += "}";
  
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("=== ESP32-S3 Peristaltic Pump Controller ===");

  // Initialize GPIO
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);

  // Initialize PWM
  uint32_t actualFreq = ledcSetup(PWM_CH, PWM_FREQ, PWM_RES);
  Serial.print("[Init] LEDC channel=");
  Serial.print(PWM_CH);
  Serial.print(" freq=");
  Serial.print(PWM_FREQ);
  Serial.print("Hz (actual=");
  Serial.print(actualFreq);
  Serial.print("Hz) res=");
  Serial.print(PWM_RES);
  Serial.println("-bit");

  ledcAttachPin(PIN_PWMA, PWM_CH);
  Serial.print("[Init] Attached PWM channel ");
  Serial.print(PWM_CH);
  Serial.print(" to pin ");
  Serial.println(PIN_PWMA);

  // Initialize Motor Driver
  motorCoast();
  Serial.println("[Init] Bringing driver out of standby...");
  digitalWrite(PIN_STBY, LOW);
  delay(10);
  digitalWrite(PIN_STBY, HIGH);
  logPinStates("        ");

  // Connect to WiFi
  connectToWiFi();

  // Setup Web Server Routes
  server.on("/", handleRoot);
  server.on("/api/control", HTTP_POST, handleControl);
  server.on("/api/status", handleStatus);
  
  // Start Web Server
  server.begin();
  Serial.println("Web server started");
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Visit http://" + WiFi.localIP().toString() + " to control the peristaltic pump");
  } else {
    Serial.println("WiFi not connected. Web server may not be accessible.");
  }
  
  Serial.println("Setup complete. System ready.");
}

void loop() {
  // Handle Web Server Requests
  server.handleClient();
  
  // Check if timed run should stop
  if (isTimedRun && currentPumpState != PUMP_STOPPED) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - pumpStartTime) / 1000; // Convert to seconds
    
    if (elapsedTime >= runDuration) {
      Serial.println("Timed run completed. Stopping pump.");
      controlPump(PUMP_STOPPED, currentSpeed, 0);
    }
  }
  
  // Can add other background tasks here
  delay(10);
}