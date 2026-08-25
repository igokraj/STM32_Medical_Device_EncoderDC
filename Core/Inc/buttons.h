#pragma once 
#include "gpio.h"

typedef struct {
  GPIO_TypeDef *port;
  uint16_t      pin;
  GPIO_PinState stableState; // last confirmed state
  uint8_t       counter; // debounce sample counter 
  uint32_t      pressStartTick; // HAL_GetTick() when the press started, 0 = not currently held
  uint32_t      lastRepeatTick; // HAL_GetTick() of the last auto-repeat event
} Button_t;

extern Button_t btnPlus;
extern Button_t btnMinus;
extern Button_t btnSwitch;

// This function is used to handle 3 buttons (+/- and switch edit mode button)
uint8_t Button_Update(Button_t *btn);