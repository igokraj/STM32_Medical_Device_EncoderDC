// **** THIS IS THE FUNCTION TO INICIALIZE STOP OF THE DC MOTOR ****

#include "main.h"
#include "tim.h"

typedef enum {
  IDLE,
  RUNNING,
  E_STOP
} SystemStatus_t;

extern volatile SystemStatus_t systemStatus;
extern volatile uint32_t       ServoStartTick;
extern volatile uint8_t        servoPending;

void TriggerEStop(void)
{
  systemStatus = E_STOP;
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
  ServoStartTick = HAL_GetTick();
  servoPending = 1;
}