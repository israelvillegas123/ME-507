/**
 * @file servo.h
 * @brief Servo control interface.
 */
#ifndef SERVO_H
#define SERVO_H

#include "main.h"

typedef struct
{
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    uint8_t angle;
} Servo_t;

void Servo_Init(Servo_t *servo);
void Servo_SetAngle(Servo_t *servo, uint8_t angle);
void Servo_WiggleEars(Servo_t *left, Servo_t *right);
void Servo_Ears_Down(Servo_t *left, Servo_t *right);
void Servo_Ears_Up(Servo_t *left, Servo_t *right);

#endif
