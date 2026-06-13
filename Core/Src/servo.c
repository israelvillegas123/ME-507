/**
 * @file servo.c
 * @brief Llama ear control functions.
 */

#include "servo.h"

#define SERVO_MIN_US 1000
#define SERVO_MAX_US 2000

/**
 * @brief Initializes a servo PWM channel.
 *
 * @param servo Pointer to the servo configuration structure.
 */
void Servo_Init(Servo_t *servo)
{
    HAL_TIM_PWM_Start(servo->htim, servo->channel);
}

/**
 * @brief Sets the servo angle.
 *
 * Converts an angle from 0 to 180 degrees into a PWM pulse width.
 *
 * @param servo Pointer to the servo configuration structure.
 * @param angle Desired servo angle in degrees.
 */
void Servo_SetAngle(Servo_t *servo, uint8_t angle)
{
    if(angle > 180)
        angle = 180;

    servo->angle = angle;
    uint32_t pulse = SERVO_MIN_US + ((SERVO_MAX_US - SERVO_MIN_US) * angle) / 180;
    __HAL_TIM_SET_COMPARE(servo->htim, servo->channel, pulse);
}

/**
 * @brief Wiggles both llama ears.
 *
 * Creates a repeating ear motion by changing both servo angles over time.
 *
 * @param left Pointer to the left ear servo.
 * @param right Pointer to the right ear servo.
 */
void Servo_WiggleEars(Servo_t *left, Servo_t *right)
{
    static uint32_t lastMove = 0;
    static int8_t dir = 1;
    static int16_t pos = 0;

    uint32_t now = HAL_GetTick();

    if(now - lastMove < 40)
        return;

    lastMove = now;

    pos += dir * 10;

    if(pos >= 100)
        dir = -1;

    if(pos <= 0)
        dir = 1;

    Servo_SetAngle(left, pos);
    Servo_SetAngle(right, 110 - pos);
}

/**
 * @brief Moves both llama ears downward.
 *
 * @param left Pointer to the left ear servo.
 * @param right Pointer to the right ear servo.
 */
void Servo_Ears_Down(Servo_t *left, Servo_t *right)
{
    Servo_SetAngle(left, 0);
    Servo_SetAngle(right, 180);
}

/**
 * @brief Moves both llama ears upward.
 *
 * @param left Pointer to the left ear servo.
 * @param right Pointer to the right ear servo.
 */
void Servo_Ears_Up(Servo_t *left, Servo_t *right)
{
    Servo_SetAngle(left, 180);
    Servo_SetAngle(right, 0);
}
