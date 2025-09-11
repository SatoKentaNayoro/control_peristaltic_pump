#include "vacuum_pump.h"

VacuumPump::VacuumPump() {
  currentState = VACUUM_STOPPED;
  currentSpeedPercent = 100;
  runDuration = 5;
  pumpStartTime = 0;
  isTimedRun = false;
  lastDuty = 0;
}

uint32_t VacuumPump::percentToDuty(uint8_t percent) const {
  if (percent > 100) percent = 100;
  return (percent * getMaxDuty()) / 100;
}

uint8_t VacuumPump::dutyToPercent(uint32_t duty) const {
  if (duty > getMaxDuty()) duty = getMaxDuty();
  return (duty * 100) / getMaxDuty();
}

void VacuumPump::begin() {
  // Initialize GPIO for vacuum pump (Channel B)
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);

  // BO1 and BO2 are 6612FNG output pins, directly connected to vacuum pump
  // No GPIO control needed for BO1/BO2
  Serial.println("[Vacuum] BO1/BO2 are 6612FNG outputs, directly connected to vacuum pump");

  // Initialize PWM
  uint32_t actualFreq = ledcSetup(PWM_CH, PWM_FREQ, PWM_RES);
  Serial.print("[Vacuum] LEDC channel=");
  Serial.print(PWM_CH);
  Serial.print(" freq=");
  Serial.print(PWM_FREQ);
  Serial.print("Hz (actual=");
  Serial.print(actualFreq);
  Serial.print("Hz) res=");
  Serial.print(PWM_RES);
  Serial.println("-bit");

  ledcAttachPin(PIN_PWMB, PWM_CH);
  Serial.print("[Vacuum] Attached PWM channel ");
  Serial.print(PWM_CH);
  Serial.print(" to pin ");
  Serial.println(PIN_PWMB);

  // Initialize Motor Driver
  motorCoast();
  Serial.println("[Vacuum] Bringing driver out of standby...");
  digitalWrite(PIN_STBY, LOW);
  delay(10);
  digitalWrite(PIN_STBY, HIGH);
  logPinStates("        ");

  Serial.println("[Vacuum] Vacuum pump initialized successfully");
}

void VacuumPump::disableDriver() {
  ledcWrite(PWM_CH, 0);
  digitalWrite(PIN_STBY, LOW);
  lastDuty = 0;
  Serial.println("[Vacuum] Driver disabled (STBY=LOW)");
}

void VacuumPump::enableDriver() {
  digitalWrite(PIN_STBY, HIGH);
  delayMicroseconds(10);  // Small delay for driver to stabilize
  Serial.println("[Vacuum] Driver enabled (STBY=HIGH)");
}

void VacuumPump::logPinStates(const char* prefix) {
  int bin1 = digitalRead(PIN_BIN1);
  int bin2 = digitalRead(PIN_BIN2);
  int stby = digitalRead(PIN_STBY);

  Serial.print(prefix);
  Serial.print(" BIN1=");
  Serial.print(bin1);
  Serial.print(" BIN2=");
  Serial.print(bin2);
  Serial.print(" STBY=");
  Serial.print(stby);
  Serial.print(" PWM(duty)=");
  Serial.print(lastDuty);
  Serial.print("/");
  Serial.print(getMaxDuty());
  Serial.print(" (");
  Serial.print(dutyToPercent(lastDuty));
  Serial.println("%)");
}

void VacuumPump::motorCoast() {
  // Proper coast: PWM=0 first, then set direction, then disable driver
  ledcWrite(PWM_CH, 0);
  digitalWrite(PIN_BIN1, LOW);
  digitalWrite(PIN_BIN2, LOW);
  disableDriver();
  lastDuty = 0;

  Serial.println("[Vacuum] Coast (freewheel)");
  logPinStates("        ");
}

void VacuumPump::motorBrake() {
  // True brake: IN1=IN2=HIGH + PWM=MAX for short time
  // Note: Use with caution, high current!
  enableDriver();
  digitalWrite(PIN_BIN1, HIGH);
  digitalWrite(PIN_BIN2, HIGH);
  ledcWrite(PWM_CH, getMaxDuty());
  lastDuty = getMaxDuty();

  Serial.println("[Vacuum] Brake (short brake) - HIGH CURRENT!");
  logPinStates("        ");
  
  // Hold brake for short time, then coast
  delay(50);  // Short brake duration
  motorCoast();
}

void VacuumPump::motorForward(uint8_t speedPercent) {
  // Safety check - ensure we don't exceed safe speed limits
  if (speedPercent > 80) {  // Limit to 80% for vacuum pump safety
    speedPercent = 80;
    Serial.println("[Vacuum] Speed limited to 80% for safety");
  }
  
  // Convert percentage to duty cycle
  uint32_t duty = percentToDuty(speedPercent);
  
  // Proper sequence: enable driver, set direction, then PWM
  enableDriver();
  digitalWrite(PIN_BIN1, HIGH);
  digitalWrite(PIN_BIN2, LOW);
  ledcWrite(PWM_CH, duty);
  lastDuty = duty;

  Serial.print("[Vacuum] Forward | speed=");
  Serial.print(speedPercent);
  Serial.print("% (duty=");
  Serial.print(duty);
  Serial.print("/");
  Serial.print(getMaxDuty());
  Serial.println(")");
  logPinStates("        ");
}

void VacuumPump::controlVacuumPump(VacuumPumpState state, uint8_t speedPercent, uint32_t duration) {
  // Safety check before any operation
  if (!isSafeToRun() && state == VACUUM_RUNNING) {
    Serial.println("[Vacuum] Safety check failed - operation blocked");
    return;
  }

  currentState = state;
  currentSpeedPercent = speedPercent;
  runDuration = duration;
  
  switch (state) {
    case VACUUM_STOPPED:
      motorCoast();
      isTimedRun = false;
      Serial.println("[Vacuum] Vacuum pump stopped");
      break;
    case VACUUM_RUNNING:
      motorForward(speedPercent);
      if (duration > 0) {
        isTimedRun = true;
        pumpStartTime = millis();
        Serial.println("[Vacuum] Vacuum pump started for " + String(duration) + " seconds");
      } else {
        isTimedRun = false;
        Serial.println("[Vacuum] Vacuum pump started (continuous)");
      }
      break;
  }
}

void VacuumPump::update() {
  // Check if timed run should stop
  if (isTimedRun && currentState != VACUUM_STOPPED) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - pumpStartTime) / 1000; // Convert to seconds
    
    if (elapsedTime >= runDuration) {
      Serial.println("[Vacuum] Timed run completed. Stopping vacuum pump.");
      controlVacuumPump(VACUUM_STOPPED, currentSpeedPercent, 0);
    }
  }
}

uint32_t VacuumPump::getRemainingTime() const {
  if (isTimedRun && currentState != VACUUM_STOPPED) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - pumpStartTime) / 1000;
    return (runDuration > elapsedTime) ? (runDuration - elapsedTime) : 0;
  }
  return 0;
}

void VacuumPump::emergencyStop() {
  Serial.println("[Vacuum] EMERGENCY STOP - Vacuum pump stopped immediately");
  disableDriver();  // PWM=0 + STBY=LOW for fastest, gentlest stop
  currentState = VACUUM_STOPPED;
  isTimedRun = false;
}

bool VacuumPump::isSafeToRun() const {
  // Add safety checks here
  // For example: temperature, pressure sensors, etc.
  // For now, always return true, but this can be expanded
  return true;
}
