#include "pump.h"

PeristalticPump::PeristalticPump() {
  currentState = PUMP_STOPPED;
  currentSpeed = 512;  // 50% of 1023
  runDuration = 5;
  pumpStartTime = 0;
  isTimedRun = false;
  lastDuty = 0;
}

void PeristalticPump::begin() {
  // Initialize GPIO
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);

  // Initialize PWM
  uint32_t actualFreq = ledcSetup(PWM_CH, PWM_FREQ, PWM_RES);
  Serial.print("[Pump] LEDC channel=");
  Serial.print(PWM_CH);
  Serial.print(" freq=");
  Serial.print(PWM_FREQ);
  Serial.print("Hz (actual=");
  Serial.print(actualFreq);
  Serial.print("Hz) res=");
  Serial.print(PWM_RES);
  Serial.println("-bit");

  ledcAttachPin(PIN_PWMA, PWM_CH);
  Serial.print("[Pump] Attached PWM channel ");
  Serial.print(PWM_CH);
  Serial.print(" to pin ");
  Serial.println(PIN_PWMA);

  // Initialize Motor Driver
  Serial.println("[Pump] Initializing motor driver...");
  digitalWrite(PIN_STBY, LOW);  // Start with driver disabled
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  ledcWrite(PWM_CH, 0);  // Set PWM to 0
  delay(10);
  digitalWrite(PIN_STBY, HIGH);  // Enable driver
  logPinStates("        ");
}

void PeristalticPump::logPinStates(const char* prefix) {
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
  Serial.print(lastDuty);
  Serial.print("/");
  Serial.print((1 << PWM_RES) - 1);
  Serial.print(" (");
  Serial.print((lastDuty * 100) / ((1 << PWM_RES) - 1));
  Serial.println("%)");
}

void PeristalticPump::motorCoast() {
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  ledcWrite(PWM_CH, 0);
  lastDuty = 0;

  Serial.println("[Motor] Coast (freewheel)");
  logPinStates("        ");
}

void PeristalticPump::motorBrake() {
  digitalWrite(PIN_AIN1, HIGH);
  digitalWrite(PIN_AIN2, HIGH);
  ledcWrite(PWM_CH, 0);
  lastDuty = 0;

  Serial.println("[Motor] Brake (short brake)");
  logPinStates("        ");
}

void PeristalticPump::motorForward(uint16_t speed) {
  digitalWrite(PIN_STBY, HIGH);  // Enable driver
  digitalWrite(PIN_AIN1, HIGH);
  digitalWrite(PIN_AIN2, LOW);
  ledcWrite(PWM_CH, speed);
  lastDuty = speed;

  Serial.print("[Motor] Forward | speed=");
  Serial.print(speed);
  Serial.print(" (");
  Serial.print((speed * 100) / ((1 << PWM_RES) - 1));
  Serial.println("%)");
  logPinStates("        ");
}

void PeristalticPump::motorReverse(uint16_t speed) {
  digitalWrite(PIN_STBY, HIGH);  // Enable driver
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, HIGH);
  ledcWrite(PWM_CH, speed);
  lastDuty = speed;

  Serial.print("[Motor] Reverse | speed=");
  Serial.print(speed);
  Serial.print(" (");
  Serial.print((speed * 100) / ((1 << PWM_RES) - 1));
  Serial.println("%)");
  logPinStates("        ");
}

void PeristalticPump::controlPump(PumpState state, uint16_t speed, uint32_t duration) {
  Serial.println("[Pump] controlPump called - State: " + String(state) + ", Speed: " + String(speed) + ", Duration: " + String(duration));
  
  currentState = state;
  currentSpeed = speed;
  runDuration = duration;
  
  switch (state) {
    case PUMP_STOPPED:
      Serial.println("[Pump] Executing STOP");
      motorCoast();
      isTimedRun = false;
      break;
    case PUMP_FORWARD:
      Serial.println("[Pump] Executing FORWARD");
      motorForward(speed);
      if (duration > 0) {
        isTimedRun = true;
        pumpStartTime = millis();
        Serial.println("[Pump] Timed run started for " + String(duration) + " seconds");
      } else {
        isTimedRun = false;
        Serial.println("[Pump] Continuous run started");
      }
      break;
    case PUMP_REVERSE:
      Serial.println("[Pump] Executing REVERSE");
      motorReverse(speed);
      if (duration > 0) {
        isTimedRun = true;
        pumpStartTime = millis();
        Serial.println("[Pump] Timed run started for " + String(duration) + " seconds");
      } else {
        isTimedRun = false;
        Serial.println("[Pump] Continuous run started");
      }
      break;
  }
  
  Serial.println("[Pump] controlPump completed");
}

void PeristalticPump::update() {
  // Check if timed run should stop
  if (isTimedRun && currentState != PUMP_STOPPED) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - pumpStartTime) / 1000; // Convert to seconds
    
    if (elapsedTime >= runDuration) {
      Serial.println("[Pump] Timed run completed. Stopping pump.");
      controlPump(PUMP_STOPPED, currentSpeed, 0);
    }
  }
}

uint32_t PeristalticPump::getRemainingTime() const {
  if (isTimedRun && currentState != PUMP_STOPPED) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - pumpStartTime) / 1000;
    return (runDuration > elapsedTime) ? (runDuration - elapsedTime) : 0;
  }
  return 0;
}
