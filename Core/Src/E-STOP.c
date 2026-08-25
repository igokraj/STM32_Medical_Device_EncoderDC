// **** THIS IS THE FUNCTION TO INITIALIZE STOP OF THE DC MOTOR ****

#include "main.h"
#include "tim.h"
#include "state.h"

void TriggerEStop(void)
{
  systemStatus = E_STOP;
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
  ServoStartTick = HAL_GetTick();
  servoPending = 1;
}