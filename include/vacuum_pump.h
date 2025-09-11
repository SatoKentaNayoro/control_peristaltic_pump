#ifndef VACUUM_PUMP_H
#define VACUUM_PUMP_H

#include <Arduino.h>

// Vacuum Pump States
enum VacuumPumpState {
  VACUUM_STOPPED,
  VACUUM_RUNNING   // Only forward direction allowed
};

class VacuumPump {
private:
  // Pin definitions for 6612FNG - Vacuum Pump (Channel B)
  const int PIN_PWMB = 14;  // PWM-capable for vacuum pump
  const int PIN_STBY = 10;  // Shared standby pin
  const int PIN_BIN1 = 15;  // BIN1 for vacuum pump
  const int PIN_BIN2 = 16;  // BIN2 for vacuum pump
  // BO1 and BO2 are 6612FNG output pins, directly connected to vacuum pump
  // BO1 -> Vacuum pump positive terminal
  // BO2 -> Vacuum pump negative terminal
  
  // PWM settings
  const int PWM_CH = 1;  // Use different channel from peristaltic pump
  const int PWM_FREQ = 20000;  // 20kHz to avoid audible noise
  const int PWM_RES = 10;  // 10-bit resolution for better control
  
  // State variables
  VacuumPumpState currentState;
  uint8_t currentSpeedPercent;  // Speed as percentage (0-100)
  uint32_t runDuration;
  unsigned long pumpStartTime;
  bool isTimedRun;
  uint32_t lastDuty;
  
  // PWM constants
  uint32_t getMaxDuty() const { return (1 << PWM_RES) - 1; }
  uint32_t percentToDuty(uint8_t percent) const;
  uint8_t dutyToPercent(uint32_t duty) const;
  
  // Private methods
  void logPinStates(const char* prefix);
  void motorCoast();
  void motorBrake();
  void motorForward(uint8_t speedPercent);
  void disableDriver();
  void enableDriver();
  
public:
  VacuumPump();
  void begin();
  void controlVacuumPump(VacuumPumpState state, uint8_t speed = 100, uint32_t duration = 0);
  void update(); // Call in main loop to handle timed runs
  
  // Getters
  VacuumPumpState getCurrentState() const { return currentState; }
  uint8_t getCurrentSpeed() const { return currentSpeedPercent; }
  uint32_t getRunDuration() const { return runDuration; }
  bool getIsTimedRun() const { return isTimedRun; }
  unsigned long getPumpStartTime() const { return pumpStartTime; }
  uint32_t getRemainingTime() const;
  
  // Safety methods
  void emergencyStop();
  bool isSafeToRun() const;
};

#endif // VACUUM_PUMP_H
