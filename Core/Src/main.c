/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "buttons.h"
#include "pid.h"
#include "stdbool.h"
#include "E-STOP.h"
#include "state.h"
#include "display.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// **** ENCODER ****
volatile uint32_t uwDirection = 0;
volatile int32_t  iCount = 0;
volatile float rpm = 0.0f;
static int32_t prevCount = 0; // previous encoder count, used to compute delta for speed calculation

// **** LID ****
volatile bool lid_open = false; // is the lid open or not? true for lid open and false for locked

// **** SYSTEM STATUS AND EDIT MODE ****
volatile SystemStatus_t systemStatus = IDLE;
volatile EditMode_t     editMode    = EDIT_SPEED;

// **** USER DESIRED PARAMETERS ****
// Desired DC motor speed
volatile int16_t        targetRPM   = 0;
// Desired DC motor work time in a single task
volatile int16_t        targetTimeSec = 0;

// **** DC MOTOR ****
#define MAX_DC_SPEED 330 // Pololu 4752 dataSheet: Rotational speed at 12 V power supply: 330 rpm

// **** PID ramp ****
#define RAMP_RATE_RPM_PER_S   60.0f // max rate of change of the setpoint (RPM per second)
static float rampedSetpoint = 0.0f; // current ramped setpoint, output of Ramp_Update

// **** TIME-BASED AUTO-STOP ****
volatile uint32_t runStartTick = 0; // timestamp (HAL_GetTick) of when RUNNING started, used to measure elapsed time
volatile uint8_t stopping = 0; // 0/1 - 1 indicates that the DC motor is slowing down to 0 rpm

// **** SERVO ****
volatile uint32_t ServoStartTick = 0; // variable for servo delay counter
volatile uint8_t servoPending = 0; // flag to notify servo if the machine finished and servo can now wait its own delay till it is opened
volatile bool servoLocked = true; // current commanded position of the lock servo (true = locked, false = open)
#define SERVO_OPEN_DELAY 10000 // How much time must pass for servo to open after the machine finished its work?
#define SERVO_MAX_WAIT 60000 // Max wait time for the motor to stop; servo opens after this time even if the motor hasn't fully stopped yet

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM6_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  /* Start the encoder interface */
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);

  /* Start TIM6 in interrupt mode */
  HAL_TIM_Base_Start_IT(&htim6);
  HAL_TIM_Base_Start(&htim3);

  /* Start PWM channels for the motor driver (IN1/IN2) */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

  Display_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    Display_Update();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */


#define COUNTS_PER_REV_OUTPUT   1920.0f   //  64 CPR x 30:1 gearbox Pololu datasheet
#define SAMPLE_TIME_S           0.01f     // 100 Hz from TIM6 -> 0,01 s

// TIM6 callback (100 Hz) - computes direction, position and motor RPM
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

// **** RPM CALCULATION ****

if (htim->Instance == TIM6) {

  uwDirection = __HAL_TIM_IS_TIM_COUNTING_DOWN(&htim2);
  iCount = (int32_t)__HAL_TIM_GET_COUNTER(&htim2) / 4;

  int32_t delta = iCount - prevCount;
  prevCount = iCount;

  rpm = (delta / COUNTS_PER_REV_OUTPUT) * (60.0f / SAMPLE_TIME_S);
}

// **** LID CHECK ****

lid_open = HAL_GPIO_ReadPin(Lid_Button_GPIO_Port, Lid_Button_Pin);

// Set systemStatus to E-STOP and zero the PWM signal if someone managed to open the lid while the system is RUNNING
if (systemStatus == RUNNING && lid_open) {
// Initialize stop of DC motor
TriggerEStop();
}

// **** BUTTONS HANDLE **** 

// Enable the change of the parameters only if the system is in IDLE
if (systemStatus == IDLE) { 

// Change of the edit mode with switch_button
if (Button_Update(&btnSwitch)) {
  editMode = (editMode == EDIT_SPEED) ? EDIT_TIME : EDIT_SPEED;
}
// Change of the rpm and work time with Button+ and Button-
if (Button_Update(&btnPlus)) {
  if (editMode == EDIT_SPEED) {
    if (targetRPM < 330) {
    targetRPM += 10;
    }
  }
 else {
  if (targetTimeSec < 3600) {
    targetTimeSec += 10;
  }
}
}
if (Button_Update(&btnMinus)) {
  if (editMode == EDIT_SPEED) {
    if (targetRPM >= 10) {
      targetRPM -= 10;
    }
  }
  else {
    if (targetTimeSec >= 10) {
      targetTimeSec -= 10;
    }
  }
}
}


// **** PWM HANDLE **** 

if (systemStatus == RUNNING) {


  // Check if the configured run time has elapsed, and if so, start the smooth stop
  if (!stopping)  {
    uint32_t elapsedSec = (HAL_GetTick() - runStartTick) / 1000;
    if (elapsedSec >= (uint32_t)targetTimeSec) {
      stopping = 1;
    }
  }
  float effectiveTarget = stopping ? 0.0f : (float)targetRPM;

  // Move the setpoint gradually, smoothly towards effectiveTarget instead of step change
  rampedSetpoint = Ramp_Update(rampedSetpoint, effectiveTarget, RAMP_RATE_RPM_PER_S, SAMPLE_TIME_S);
  // Compare the rampedSetpoint (goal for right now) with the actual measured speed
  float pidOutput = PID_Compute(rampedSetpoint, rpm);


  /* Check systemStatus again right before writing to CCR - if EXTI (higher priority than this callback) just fired and switched to E_STOP, it already zeroed CCR; this re-check prevents the write below from overwriting that zero */
  if (systemStatus == RUNNING) {
  if (pidOutput > 0) {
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)pidOutput);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
  }
  else {
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);   // never reverse
  }

  // Finish the smooth stop and return to IDLE once rampedSetpoint has reached ~0
  if (stopping && rampedSetpoint <= 0.5f) {
    systemStatus = IDLE;
    stopping = 0;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
      ServoStartTick = HAL_GetTick();
      servoPending = 1;
    }
  }
}

// **** SERVO OPEN DELAY ****

// Wait for the set time before opening the lock
if (servoPending) {
  uint32_t now = HAL_GetTick();
  bool timedOut = (now - ServoStartTick >= SERVO_MAX_WAIT); 
  bool motorStopped = (rpm < 1.0f && rpm > -1.0f);

/* Open once the minimum delay has passed AND either the motor is confirmed stopped, or the max wait timed out (this is an extra protection in case rpm never settles, e.g. sensor noise) */
  if (now - ServoStartTick >= SERVO_OPEN_DELAY && (motorStopped || timedOut)) {

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 3000);
    /* PSC=27, ARR=59999
    tick = 28 / 84 000 000 ≈ 0,333 µs
    For example: 0,333µs × 3000 = 999µs ≈ 1ms
    0°   → 3000 (1ms)
    90°  → 4500 (1,5ms)
    180° → 6000 (2 ms) */
    servoLocked = false;
    servoPending = 0;
  }
}

}

// **** SYSTEM STATE MACHINE ****

// This function in ISR is used to start or stop the system (and to handle the state machine)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  static uint32_t last_press = 0;

  if (GPIO_Pin == Start_Button_Pin) {

    uint32_t now = HAL_GetTick();
    if (now - last_press < 200) return;
    last_press = now;

    // Pressing the button stops the system if it is in RUNNING state
    if (systemStatus == RUNNING) {
    // Initialize stop of DC motor
    TriggerEStop();
    }
    // Pressing the button starts the system if it is in IDLE state, resets the ramp/timer state for a fresh run and also locks the machine with servo
    else if (systemStatus == IDLE) {
      if (!lid_open) {
        if (targetRPM != 0 && targetTimeSec != 0) {
        // Lock the servo, and cancel any pending "open" countdown left over from a previous cycle
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 4500);
      servoLocked = true;
      servoPending = 0;
      systemStatus = RUNNING;
      rampedSetpoint = 0.0f;
      runStartTick = HAL_GetTick();
      stopping = 0;
        }
        else {
          // User did not specify desired RPM or work time
          Display_ShowMessage("SET RPM AND TIME");
        }
    }
    else {
      // lid is open - just ignore this request
      Display_ShowMessage("CLOSE THE LID");
    }
  }
    else {
      // Require a separate press to leave E_STOP - won't jump straight back to RUNNING
      systemStatus = IDLE;
    }
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
