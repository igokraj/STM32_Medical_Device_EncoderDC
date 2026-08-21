#include "buttons.h"

#define DEBOUNCE_TICKS  3

// *** THE CODE BELOW IS USED TO HANDLE 3 BUTTONS (+/- and switch button) IN ISR ***

Button_t btnPlus   = { Button_B6_GPIO_Port,     Button_B6_Pin,     GPIO_PIN_SET, 0 };
Button_t btnMinus  = { Button__GPIO_Port,       Button__Pin,       GPIO_PIN_SET, 0 };
Button_t btnSwitch = { Switch_Button_GPIO_Port, Switch_Button_Pin, GPIO_PIN_SET, 0 };

/* Returns 1 on a debounced press event (falling edge), else 0 */
uint8_t Button_Update(Button_t *btn)
{
  GPIO_PinState raw = HAL_GPIO_ReadPin(btn->port, btn->pin);

  // If the reading is the same as last confirmed state, nothing is happening
  if (raw == btn->stableState)
  {
    btn->counter = 0;
    return 0;
  }

  // The reading is different - it might be a real press, or just bouncing
  btn->counter = btn->counter + 1;

  // Wait until the new reading has been stable for DEBOUNCE_TICKS in a row
  if (btn->counter >= DEBOUNCE_TICKS)
  {
    btn->stableState = raw;
    btn->counter = 0;

    // Only report an event when the button became PRESSED (pin reads LOW)
    if (raw == GPIO_PIN_RESET)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  // Not stable long enough yet
  return 0;
}