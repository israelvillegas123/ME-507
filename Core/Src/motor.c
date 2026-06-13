/**
 * @file motor.c
 * @brief DC motor control functions.
 */
#include "motor.h"

/**
 * @brief Initializes a DC motor PWM interface.
 *
 * Starts both PWM channels used by the motor driver and stops the motor.
 *
 * @param motor Pointer to the motor configuration structure.
 */
void Motor_Init(motor_t *motor)
{
    HAL_TIM_PWM_Start(motor->timer, motor->channel1);
    HAL_TIM_PWM_Start(motor->timer, motor->channel2);

    Motor_Stop(motor);
}

/**
 * @brief Enables the DRV8833 motor driver.
 *
 * Sets the motor driver's nSLEEP pin high so the motors can operate.
 */
void Motor_Enable(void)
{
    HAL_GPIO_WritePin(Motor_NSleep_GPIO_Port, Motor_NSleep_Pin, GPIO_PIN_SET);
    HAL_Delay(2);
}

/**
 * @brief Disables the DRV8833 motor driver.
 *
 * Sets the motor driver's nSLEEP pin low to place the driver into sleep mode.
 */
void Motor_Disable(void)
{
    HAL_GPIO_WritePin(Motor_NSleep_GPIO_Port, Motor_NSleep_Pin, GPIO_PIN_RESET);
}

/**
 * @brief Checks whether the motor driver fault pin is active.
 *
 * @return 1 if a fault is active, 0 otherwise.
 */
uint8_t Motor_FaultActive(void)
{
    return HAL_GPIO_ReadPin(Motor_NFault_GPIO_Port, Motor_NFault_Pin) == GPIO_PIN_RESET;
}

/**
 * @brief Sets motor speed and direction using PWM duty cycle.
 *
 * Positive values rotate the motor forward, negative values rotate it in reverse,
 * and zero stops the motor.
 *
 * @param motor Pointer to the motor configuration structure.
 * @param duty_percent Duty cycle from -100.0 to 100.0 percent.
 */
void Motor_Set_Duty(motor_t *motor, float duty_percent)
{
    if (duty_percent > 100.0f) duty_percent = 100.0f;
    if (duty_percent < -100.0f) duty_percent = -100.0f;

    float abs_duty = duty_percent;
    if (abs_duty < 0.0f)
    {
        abs_duty = -abs_duty;
    }

    uint32_t pwm = (uint32_t)(abs_duty * motor->pwm_max / 100.0f);

    if (duty_percent > 0.0f)
    {
        __HAL_TIM_SET_COMPARE(motor->timer, motor->channel1, pwm);
        __HAL_TIM_SET_COMPARE(motor->timer, motor->channel2, 0);
    }
    else if (duty_percent < 0.0f)
    {
        __HAL_TIM_SET_COMPARE(motor->timer, motor->channel1, 0);
        __HAL_TIM_SET_COMPARE(motor->timer, motor->channel2, pwm);
    }
    else
    {
        __HAL_TIM_SET_COMPARE(motor->timer, motor->channel1, 0);
        __HAL_TIM_SET_COMPARE(motor->timer, motor->channel2, 0);
    }
}

/**
 * @brief Stops the selected motor.
 *
 * @param motor Pointer to the motor configuration structure.
 */
void Motor_Stop(motor_t *motor)
{
    Motor_Set_Duty(motor, 0.0f);
}
