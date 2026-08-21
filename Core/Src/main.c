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
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
volatile uint32_t uwDirection = 0;
volatile int32_t  iCount = 0;
volatile float rpm = 0.0f;
static int32_t prevCount = 0; // previous encoder count, used to compute delta for speed calculation

typedef enum {
  IDLE,
  RUNNING,
  E_STOP
} SystemStatus_t;

volatile SystemStatus_t systemStatus = IDLE;

typedef enum {
  EDIT_SPEED,
  EDIT_TIME
} EditMode_t;

volatile EditMode_t     editMode    = EDIT_SPEED;

// Desired DC motor speed
volatile int16_t        targetRPM   = 0;
// Desired DC motor work time in a single task
volatile int16_t        targetTimeSec = 0;





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
  /* USER CODE BEGIN 2 */

  /* Start the encoder interface */
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);

  /* Start TIM6 in interrupt mode */
  HAL_TIM_Base_Start_IT(&htim6);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

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
// *** THE CODE BELOW IS USED TO HANDLE 3 BUTTONS (+/- and switch button) IN ISR ***
typedef struct {
  GPIO_TypeDef *port;
  uint16_t      pin;
  GPIO_PinState stableState;
  uint8_t       counter;
} Button_t;

#define DEBOUNCE_TICKS  3   // 3 x 10ms (tick TIM6) = 30ms

static Button_t btnPlus   = { Button_B6_GPIO_Port,     Button_B6_Pin,     GPIO_PIN_SET, 0 };
static Button_t btnMinus  = { Button__GPIO_Port,       Button__Pin,       GPIO_PIN_SET, 0 };
static Button_t btnSwitch = { Switch_Button_GPIO_Port, Switch_Button_Pin, GPIO_PIN_SET, 0 };

/* Returns 1 on a debounced press event (falling edge), else 0 */
static uint8_t Button_Update(Button_t *btn)
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

#define COUNTS_PER_REV_OUTPUT   1920.0f   //  64 CPR x 30:1 gearbox Pololu datasheet
#define SAMPLE_TIME_S           0.01f     // 100 Hz from TIM6 -> 0,01 s

// TIM6 callback (100 Hz) - computes direction, position and motor RPM
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
if (htim->Instance == TIM6) {

  uwDirection = __HAL_TIM_IS_TIM_COUNTING_DOWN(&htim2);
  iCount = (int32_t)__HAL_TIM_GET_COUNTER(&htim2) / 4;

  int32_t delta = iCount - prevCount;
  prevCount = iCount;

  rpm = (delta / COUNTS_PER_REV_OUTPUT) * (60.0f / SAMPLE_TIME_S);
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
    targetRPM += 10;
  }
  else {
    targetTimeSec += 10;
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
}

// This funtion in ISR is used to start of stop the system (and to handle the state machine)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  static uint32_t last_press = 0;

  if (GPIO_Pin == Start_Button_Pin) {

    uint32_t now = HAL_GetTick();
    if (now - last_press < 200) return;
    last_press = now;

    // Pressing the button stops the system if it is RUNNING state
    if (systemStatus == RUNNING) {
      systemStatus = E_STOP;
    }
    // Pressing the button starts the system if it is in IDLE state
    else if (systemStatus == IDLE) {
      systemStatus = RUNNING;
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
