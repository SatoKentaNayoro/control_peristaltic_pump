#ifndef PUMP_H
#define PUMP_H

#include <Arduino.h>

// Peristaltic Pump States
enum PumpState {
  PUMP_STOPPED,
  PUMP_FORWARD,  // Forward rotation - Sample intake
  PUMP_REVERSE   // Reverse rotation - Liquid extraction
};

class PeristalticPump {
private:
  // Pin definitions
  const int PIN_PWMA = 13;  // PWM-capable
  const int PIN_STBY = 10;
  const int PIN_AIN1 = 11;
  const int PIN_AIN2 = 12;
  
  // PWM settings
  const int PWM_CH = 2;  // Use different channel from vacuum pump
  const int PWM_FREQ = 20000;
  const int PWM_RES = 10;  // Match vacuum pump resolution
  
  // State variables
  PumpState currentState;
  uint16_t currentSpeed;  // 0-1023 for 10-bit PWM
  uint32_t runDuration;
  unsigned long pumpStartTime;
  bool isTimedRun;
  uint32_t lastDuty;
  
  // Private methods
  void logPinStates(const char* prefix);
  void motorCoast();
  void motorBrake();
  void motorForward(uint16_t speed);
  void motorReverse(uint16_t speed);
  
public:
  PeristalticPump();
  void begin();
  void controlPump(PumpState state, uint16_t speed = 512, uint32_t duration = 0);
  void update(); // Call in main loop to handle timed runs
  
  // Getters
  PumpState getCurrentState() const { return currentState; }
  uint16_t getCurrentSpeed() const { return currentSpeed; }
  uint32_t getRunDuration() const { return runDuration; }
  bool getIsTimedRun() const { return isTimedRun; }
  unsigned long getPumpStartTime() const { return pumpStartTime; }
  uint32_t getRemainingTime() const;
};

#endif // PUMP_H
