/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "motor.h"
#include "servo.h"
#include "usbd_cdc_if.h"
#include "task_distance.h"
#include "usb_print.h"
#include "screen.h"
#include "servo.h"


#include <string.h>
#include <stdio.h>
#include <stdarg.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ENABLE_MOTOR_SERVO_INIT 0

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */

static motor_t left_motor =
{
    .timer = &htim1,
    .channel1 = TIM_CHANNEL_1,
    .channel2 = TIM_CHANNEL_2,
    .pwm_max = 4199
};

static motor_t right_motor =
{
    .timer = &htim4,
    .channel1 = TIM_CHANNEL_3,
    .channel2 = TIM_CHANNEL_4,
    .pwm_max = 4199
};

static Servo_t left_servo =
{
    .htim = &htim2,
    .channel = TIM_CHANNEL_3,
    .angle = 90
};

static Servo_t right_servo =
{
    .htim = &htim2,
    .channel = TIM_CHANNEL_4,
    .angle = 90
};

volatile int32_t left_encoder_count = 0;
volatile int32_t right_encoder_count = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */
static void Debug_XSHUT_GPIO_After_USB_Start(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void Debug_XSHUT_GPIO_After_USB_Start(void)
{
    HAL_GPIO_WritePin(TOF1_XShut_GPIO_Port, TOF1_XShut_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TOF2_XShut_GPIO_Port, TOF2_XShut_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);

    USB_Printf("XSHUT low readback: TOF1=%d TOF2=%d\r\n",
               HAL_GPIO_ReadPin(TOF1_XShut_GPIO_Port, TOF1_XShut_Pin),
               HAL_GPIO_ReadPin(TOF2_XShut_GPIO_Port, TOF2_XShut_Pin));

    HAL_GPIO_WritePin(TOF1_XShut_GPIO_Port, TOF1_XShut_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TOF2_XShut_GPIO_Port, TOF2_XShut_Pin, GPIO_PIN_SET);
    HAL_Delay(100);

    USB_Printf("XSHUT high readback: TOF1=%d TOF2=%d\r\n",
               HAL_GPIO_ReadPin(TOF1_XShut_GPIO_Port, TOF1_XShut_Pin),
               HAL_GPIO_ReadPin(TOF2_XShut_GPIO_Port, TOF2_XShut_Pin));

    HAL_GPIO_WritePin(TOF1_XShut_GPIO_Port, TOF1_XShut_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TOF2_XShut_GPIO_Port, TOF2_XShut_Pin, GPIO_PIN_RESET);
    HAL_Delay(150);
}


/**
 * @brief Runs sleep mode behavior.
 *
 * Stops normal interaction by showing sleeping eyes and lowering the llama ears.
 */
static void mode_sleep(void)
{
	screen_AnimateSleepEyes();
	Servo_Ears_Down(&left_servo, &right_servo);

}

/**
 * @brief Runs pet mode behavior.
 *
 * Displays happy eyes and wiggles the llama ears.
 */
static void mode_pet(void)
{
	screen_AnimatePetEyes();
	Servo_WiggleEars(&left_servo, &right_servo);


}

/**
 * @brief Runs normal wonder mode behavior.
 *
 * Displays animated eyes and raises the llama ears.
 */
static void mode_wonder(void)
{
	screen_AnimateEyes();
	Servo_Ears_Up(&left_servo, &right_servo);

}

/**
 * @brief Runs falling or edge-detection behavior.
 *
 * Displays a scared face and raises the ears in an alert position.
 */
static void mode_falling(void)
{
	screen_AnimateScaredFace();
	Servo_Ears_Up(&left_servo, &right_servo);

}
/**
 * @brief Reads the active-low user switch with debounce.
 *
 * @return 1 if the switch is pressed, 0 otherwise.
 */
static uint8_t Switch_IsOn(void)

    if (HAL_GPIO_ReadPin(Button1_GPIO_Port, Button1_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(20);  

        if (HAL_GPIO_ReadPin(Button1_GPIO_Port, Button1_Pin) == GPIO_PIN_RESET)
        {
            return 1;
        }
    }

    return 0;
}

/* USER CODE END 0 */

#define CONTROL_DT_MS 20
#define LEFT_TARGET_CPS  250.0f
#define RIGHT_TARGET_CPS 250.0f
#define KP 1.2f
#define KI 0.08f

static float left_cmd = 50.0f;
static float right_cmd = 50.0f;
static float left_integral = 0.0f;
static float right_integral = 0.0f;

/**
 * @brief Updates closed-loop motor speed control.
 *
 * Estimates wheel speed using encoder count changes and adjusts motor PWM
 * using a PI controller.
 *
 * @param left_target_cps Desired left wheel speed in encoder counts per second.
 * @param right_target_cps Desired right wheel speed in encoder counts per second.
 */
static void Motor_ClosedLoop_Update(float left_target_cps, float right_target_cps)
    static uint32_t last_time = 0;
    static int32_t last_left_count = 0;
    static int32_t last_right_count = 0;

    uint32_t now = HAL_GetTick();

    if (last_time == 0)
    {
        last_time = now;
        last_left_count = left_encoder_count;
        last_right_count = right_encoder_count;
        return;
    }

    if (now - last_time < CONTROL_DT_MS)
        return;

    int32_t left_now = left_encoder_count;
    int32_t right_now = right_encoder_count;

    int32_t left_delta = left_now - last_left_count;
    int32_t right_delta = right_now - last_right_count;

    float dt = (now - last_time) / 1000.0f;

    float left_speed = left_delta / dt;
    float right_speed = right_delta / dt;

    float left_error = left_target_cps - left_speed;
    float right_error = right_target_cps - right_speed;

    left_integral += left_error * dt;
    right_integral += right_error * dt;

    left_cmd += KP * left_error + KI * left_integral;
    right_cmd += KP * right_error + KI * right_integral;

    if (left_cmd > 100.0f) left_cmd = 100.0f;
    if (left_cmd < -100.0f) left_cmd = -100.0f;
    if (right_cmd > 100.0f) right_cmd = 100.0f;
    if (right_cmd < -100.0f) right_cmd = -100.0f;

    Motor_Set_Duty(&left_motor, left_cmd);
    Motor_Set_Duty(&right_motor, right_cmd);

    last_left_count = left_now;
    last_right_count = right_now;
    last_time = now;
}

/**
 * @brief Resets the closed-loop motor controller.
 *
 * Clears integral terms and restores initial motor command values.
 */
static void Motor_ClosedLoop_Reset(void)
{
    left_integral = 0.0f;
    right_integral = 0.0f;
    left_cmd = 50.0f;
    right_cmd = 50.0f;
}

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();


  /* USER CODE BEGIN 2 */
	#if USB_DEBUG_ENABLED
	  MX_USB_DEVICE_Init();
	  HAL_Delay(1500);
	  USB_Print("\r\nUSB CDC started\r\n");
	#endif



  task_distance_initialize();



  Motor_Init(&left_motor);
  Motor_Init(&right_motor);
  Motor_Enable();

  Servo_Init(&left_servo);
  Servo_SetAngle(&left_servo, 180);
  Servo_Init(&right_servo);
  Servo_SetAngle(&right_servo, 0);



  screen_Init(&hi2c1);
  /* USER CODE END 2 */

  while (1)
  {
	    if (Switch_IsOn())
	    {
	    	mode_sleep();
	    	Motor_Set_Duty(&left_motor, 0.0f);
	    	Motor_Set_Duty(&right_motor, 0.0f);
	    	continue;
	    }
	    TOF_Measurement_t tof1_measure;
	    TOF_Measurement_t tof2_measure;

	    TOF_ReadTwo(&tof1_measure, &tof2_measure);
	    if (tof2_measure.distance_mm < 60)
	    {
	    	mode_pet();
	    	continue;
	    } else if(tof1_measure.distance_mm > 200 || tof2_measure.distance_mm > 200)
	    {
	    	mode_falling();
        Motor_ClosedLoop_Reset();
        Motor_Set_Duty(&left_motor, -100.0f);
        Motor_Set_Duty(&right_motor, 100.0f);
	    	continue;
	    }
	    mode_wonder();
      Motor_ClosedLoop_Update(LEFT_TARGET_CPS, RIGHT_TARGET_CPS);

	    HAL_Delay(10);
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM1_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 4199;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim1);
}

static void MX_TIM2_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 19999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim2);
}

static void MX_TIM4_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 4199;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim4);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(Motor_NSleep_GPIO_Port, Motor_NSleep_Pin, GPIO_PIN_SET);

  HAL_GPIO_WritePin(TOF1_XShut_GPIO_Port, TOF1_XShut_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TOF2_XShut_GPIO_Port, TOF2_XShut_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = Button2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Button2_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = Motor_NFault_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Motor_NFault_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = Motor_NSleep_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Motor_NSleep_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TOF1_XShut_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TOF1_XShut_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TOF2_XShut_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TOF2_XShut_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = Button1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Button1_GPIO_Port, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
        {
            left_encoder_count++;
        }
        else
        {
            left_encoder_count--;
        }
    }
    else if (GPIO_Pin == GPIO_PIN_6)
    {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_SET)
        {
            right_encoder_count++;
        }
        else
        {
            right_encoder_count--;
        }
    }
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
