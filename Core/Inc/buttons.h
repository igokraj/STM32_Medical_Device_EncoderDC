#pragma once 
#include "gpio.h"

typedef struct {
  GPIO_TypeDef *port;
  uint16_t      pin;
  GPIO_PinState stableState;
  uint8_t       counter;
} Button_t;

extern Button_t btnPlus;
extern Button_t btnMinus;
extern Button_t btnSwitch;

// This function is used to handle 3 buttons (+/- and switch edit mode button)
uint8_t Button_Update(Button_t *btn);