/**
 * @file screen.h
 * @brief SSD1306 OLED display interface.
 */
#ifndef INC_SCREEN_H_
#define INC_SCREEN_H_

#include "main.h"
#include <stdint.h>
#include <string.h>

#define screen_WIDTH   128
#define screen_HEIGHT  64
#define screen_ADDR    (0x3C << 1)

void screen_Init(I2C_HandleTypeDef *hi2c);
void screen_Clear(void);
void screen_UpdateScreen(void);
void screen_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void screen_AnimateEyes(void);
void screen_AnimateSleepEyes(void);
void screen_AnimatePetEyes(void);
void screen_AnimateScaredFace(void);


#endif
