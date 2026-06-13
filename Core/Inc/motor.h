/**
 * @file motor.h
 * @brief Motor control interface.
 */
#ifndef INC_MOTOR_H_
#define INC_MOTOR_H_

#include "main.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef struct
{
    TIM_HandleTypeDef *timer;
    uint32_t channel1;
    uint32_t channel2;
    uint32_t pwm_max;
} motor_t;

void Motor_Init(motor_t *motor);
void Motor_Enable(void);
void Motor_Disable(void);
uint8_t Motor_FaultActive(void);
void Motor_Set_Duty(motor_t *motor, float duty_percent);
void Motor_Stop(motor_t *motor);

#endif /* INC_MOTOR_H_ */
