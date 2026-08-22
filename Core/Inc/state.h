#pragma once

#include "main.h"
#include <stdbool.h>

typedef enum {
  IDLE,
  RUNNING,
  E_STOP
} SystemStatus_t;

typedef enum {
  EDIT_SPEED,
  EDIT_TIME
} EditMode_t;

extern volatile SystemStatus_t systemStatus;
extern volatile EditMode_t     editMode;
extern volatile int16_t        targetRPM;
extern volatile int16_t        targetTimeSec;
extern volatile float          rpm;
extern volatile bool           lid_open;
extern volatile uint8_t        stopping;
extern volatile uint32_t       runStartTick;

extern volatile uint32_t ServoStartTick;
extern volatile uint8_t  servoPending;
extern volatile bool     servoLocked;
