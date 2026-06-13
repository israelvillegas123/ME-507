# Code Functions {#code_functions}

This page lists the main documented firmware functions used in the Desk Buddy project.

## Motor Control

- \link Motor_Init Motor_Init() \endlink
- \link Motor_Enable Motor_Enable() \endlink
- \link Motor_Disable Motor_Disable() \endlink
- \link Motor_Set_Duty Motor_Set_Duty() \endlink
- \link Motor_Stop Motor_Stop() \endlink

## Llama Ear Servo Control

- \link Servo_Init Servo_Init() \endlink
- \link Servo_SetAngle Servo_SetAngle() \endlink
- \link Servo_WiggleEars Servo_WiggleEars() \endlink
- \link Servo_Ears_Down Servo_Ears_Down() \endlink
- \link Servo_Ears_Up Servo_Ears_Up() \endlink

## OLED Display

- \link screen_Init screen_Init() \endlink
- \link screen_Clear screen_Clear() \endlink
- \link screen_UpdateScreen screen_UpdateScreen() \endlink
- \link screen_DrawPixel screen_DrawPixel() \endlink
- \link screen_AnimateEyes screen_AnimateEyes() \endlink
- \link screen_AnimateSleepEyes screen_AnimateSleepEyes() \endlink
- \link screen_AnimatePetEyes screen_AnimatePetEyes() \endlink
- \link screen_AnimateScaredFace screen_AnimateScaredFace() \endlink

## Distance Sensors

- \link task_distance_initialize task_distance_initialize() \endlink
- \link TOF_I2C_Scan_With_Both_On TOF_I2C_Scan_With_Both_On() \endlink
- \link TOF_InitTwo TOF_InitTwo() \endlink
- \link TOF_ReadTwo TOF_ReadTwo() \endlink
## Main Control Functions

- \link Switch_IsOn Switch_IsOn() \endlink
- \link mode_sleep mode_sleep() \endlink
- \link mode_pet mode_pet() \endlink
- \link mode_wonder mode_wonder() \endlink
- \link mode_falling mode_falling() \endlink
- \link Motor_ClosedLoop_Update Motor_ClosedLoop_Update() \endlink
- \link Motor_ClosedLoop_Reset Motor_ClosedLoop_Reset() \endlink
