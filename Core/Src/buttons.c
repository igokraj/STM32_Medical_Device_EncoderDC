#include "buttons.h"

#define DEBOUNCE_TICKS  2   // number of consecutive stable samples required before a state change counts

// These values are used in mechanism for rapidly increasing the value while holding down the button
#define HOLD_DELAY_MS 500 // -> How long user needs to hold the button pressed for the mechanism to initialize (button pressed ... 500 ms ... mechanism initialization)
#define REPEAT_INTERVAL_MS 100 // -> Intervals between value changes while the mechanism is running (100 RPM ... 150 ms ... 200 RPM)



// *** THE CODE BELOW IS USED TO HANDLE 3 BUTTONS (+/- and switch button) ***

Button_t btnPlus   = { Button_B6_GPIO_Port,     Button_B6_Pin,     GPIO_PIN_SET, 0, 0, 0 };
Button_t btnMinus  = { Button__GPIO_Port,       Button__Pin,       GPIO_PIN_SET, 0, 0, 0 };
Button_t btnSwitch = { Switch_Button_GPIO_Port, Switch_Button_Pin, GPIO_PIN_SET, 0, 0, 0 };

/* Returns 1 on a debounced press event, or on an auto-repeat tick while held, else 0 */
uint8_t Button_Update(Button_t *btn)
{
  GPIO_PinState raw = HAL_GPIO_ReadPin(btn->port, btn->pin);
  uint32_t now = HAL_GetTick();

  // The reading is changing - it might be a real press/release, or just bouncing
  if (raw != btn->stableState)
  {
    btn->counter = btn->counter + 1;

    // Wait until the new reading has been stable for DEBOUNCE_TICKS in a row
    if (btn->counter >= DEBOUNCE_TICKS)
    {
      btn->stableState = raw;
      btn->counter = 0;

      if (raw == GPIO_PIN_RESET)
      {
        // Fresh press - remember when it started, report it right away
        btn->pressStartTick = now;
        btn->lastRepeatTick = now;
        return 1;
      }
      else
      {
        // Released
        btn->pressStartTick = 0;
      }
    }
    return 0;
  }

  // Reading is stable - nothing changing, reset the debounce counter
  btn->counter = 0;

  // Still held down - check whether it's time for an auto-repeat event
  if (btn->stableState == GPIO_PIN_RESET && btn->pressStartTick != 0)
  {
    if (now - btn->pressStartTick >= HOLD_DELAY_MS)
    {
      if (now - btn->lastRepeatTick >= REPEAT_INTERVAL_MS)
      {
        btn->lastRepeatTick = now;
        return 1;
      }
    }
  }

  return 0;
}