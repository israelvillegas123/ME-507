/**
 * @file screen.c
 * @brief OLED display graphics and animations.
 */

#include "screen.h"
#include <string.h>

static I2C_HandleTypeDef *screen_i2c;
static uint8_t screen_Buffer[screen_WIDTH * screen_HEIGHT / 8];

static void screen_WriteCommand(uint8_t command)
{
    uint8_t data[2] = {0x00, command};
    HAL_I2C_Master_Transmit(screen_i2c, screen_ADDR, data, 2, HAL_MAX_DELAY);
}

/**
 * @brief Initializes the SSD1306 OLED display.
 *
 * Sends the required startup commands over I2C, clears the screen buffer,
 * and updates the display.
 *
 * @param hi2c Pointer to the I2C handle used by the OLED display.
 */
void screen_Init(I2C_HandleTypeDef *hi2c)
{
    screen_i2c = hi2c;
    HAL_Delay(100);

    screen_WriteCommand(0xAE);
    screen_WriteCommand(0x20);
    screen_WriteCommand(0x00);
    screen_WriteCommand(0xB0);
    screen_WriteCommand(0xC8);
    screen_WriteCommand(0x00);
    screen_WriteCommand(0x10);
    screen_WriteCommand(0x40);
    screen_WriteCommand(0x81);
    screen_WriteCommand(0xFF);
    screen_WriteCommand(0xA1);
    screen_WriteCommand(0xA6);
    screen_WriteCommand(0xA8);
    screen_WriteCommand(0x3F);
    screen_WriteCommand(0xA4);
    screen_WriteCommand(0xD3);
    screen_WriteCommand(0x00);
    screen_WriteCommand(0xD5);
    screen_WriteCommand(0xF0);
    screen_WriteCommand(0xD9);
    screen_WriteCommand(0x22);
    screen_WriteCommand(0xDA);
    screen_WriteCommand(0x12);
    screen_WriteCommand(0xDB);
    screen_WriteCommand(0x20);
    screen_WriteCommand(0x8D);
    screen_WriteCommand(0x14);
    screen_WriteCommand(0xAF);

    screen_Clear();
    screen_UpdateScreen();
}

/**
 * @brief Clears the OLED screen buffer.
 */
void screen_Clear(void)
{
    memset(screen_Buffer, 0, sizeof(screen_Buffer));
}

/**
 * @brief Sends the screen buffer to the OLED display.
 */
void screen_UpdateScreen(void)
{
    for(uint8_t page = 0; page < 8; page++)
    {
        screen_WriteCommand(0xB0 + page);
        screen_WriteCommand(0x00);
        screen_WriteCommand(0x10);

        uint8_t data[129];
        data[0] = 0x40;
        memcpy(&data[1], &screen_Buffer[screen_WIDTH * page], screen_WIDTH);

        HAL_I2C_Master_Transmit(screen_i2c, screen_ADDR, data, 129, HAL_MAX_DELAY);
    }
}

/**
 * @brief Draws a single pixel in the screen buffer.
 *
 * @param x Pixel x-coordinate.
 * @param y Pixel y-coordinate.
 * @param color Pixel state, where 1 is on and 0 is off.
 */
void screen_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if(x >= screen_WIDTH || y >= screen_HEIGHT) return;

    if(color)
        screen_Buffer[x + (y / 8) * screen_WIDTH] |= 1 << (y % 8);
    else
        screen_Buffer[x + (y / 8) * screen_WIDTH] &= ~(1 << (y % 8));
}

static void screen_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
    for(int16_t i = x; i < x + w; i++)
    {
        for(int16_t j = y; j < y + h; j++)
        {
            if(i >= 0 && i < screen_WIDTH && j >= 0 && j < screen_HEIGHT)
            {
                screen_DrawPixel((uint8_t)i, (uint8_t)j, color);
            }
        }
    }
}

static void screen_FillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color)
{
    for(int16_t y = -r; y <= r; y++)
    {
        for(int16_t x = -r; x <= r; x++)
        {
            if(x * x + y * y <= r * r)
            {
                if(x0 + x >= 0 && x0 + x < screen_WIDTH &&
                   y0 + y >= 0 && y0 + y < screen_HEIGHT)
                {
                    screen_DrawPixel((uint8_t)(x0 + x), (uint8_t)(y0 + y), color);
                }
            }
        }
    }
}


static void screen_DrawEye(uint8_t x, uint8_t y, uint8_t blink)
{
    if(blink)
    {
        screen_FillRect(x, y + 18, 40, 4, 1);
        return;
    }

    screen_FillRect(x + 8, y + 4, 24, 40, 1);
    screen_FillCircle(x + 20, y + 8, 12, 1);
    screen_FillCircle(x + 20, y + 40, 12, 1);

    screen_FillCircle(x + 14, y + 12, 4, 0);
    screen_FillCircle(x + 25, y + 22, 3, 0);
    screen_FillRect(x + 12, y + 35, 16, 3, 0);
}

static void screen_DrawEyes(uint8_t blink)
{
    screen_Clear();
    screen_DrawEye(12, 8, blink);
    screen_DrawEye(76, 8, blink);
    screen_UpdateScreen();
}

/**
 * @brief Displays the normal animated blinking eyes.
 */
void screen_AnimateEyes(void)
{
    static uint32_t lastFrame = 0;
    static uint32_t lastBlinkTime = 0;
    static uint32_t blinkStartTime = 0;
    static uint8_t blink = 0;

    uint32_t now = HAL_GetTick();

    if(now - lastFrame < 40) return;
    lastFrame = now;

    if(!blink && now - lastBlinkTime >= 3000)
    {
        blink = 1;
        blinkStartTime = now;
    }

    if(blink)
    {
        screen_DrawEyes(1);

        if(now - blinkStartTime >= 150)
        {
            blink = 0;
            lastBlinkTime = now;
        }
    }
    else
    {
        screen_DrawEyes(0);
    }
}

static void screen_DrawZ(int16_t x, int16_t y)
{
    if(y < -10 || y > screen_HEIGHT) return;

    screen_FillRect(x, y, 8, 2, 1);
    screen_FillRect(x + 6, y + 2, 2, 2, 1);
    screen_FillRect(x + 4, y + 4, 2, 2, 1);
    screen_FillRect(x + 2, y + 6, 2, 2, 1);
    screen_FillRect(x, y + 8, 8, 2, 1);
}

static void screen_DrawSleepEyes(int16_t z_offset)
{
    screen_Clear();

    screen_FillRect(18, 46, 36, 4, 1);
    screen_FillRect(74, 46, 36, 4, 1);

    screen_DrawPixel(17, 45, 1);
    screen_DrawPixel(16, 44, 1);
    screen_DrawPixel(54, 45, 1);
    screen_DrawPixel(55, 44, 1);

    screen_DrawPixel(73, 45, 1);
    screen_DrawPixel(72, 44, 1);
    screen_DrawPixel(110, 45, 1);
    screen_DrawPixel(111, 44, 1);

    screen_DrawZ(84, 18 - z_offset);
    screen_DrawZ(112, 8 - z_offset);
    screen_DrawZ(92, 34 - z_offset);

    screen_UpdateScreen();
}

/**
 * @brief Displays happy pet-mode eyes with floating hearts.
 */
void screen_AnimatePetEyes(void)
{
    static uint32_t lastFrame = 0;
    static uint8_t z_offset = 0;

    uint32_t now = HAL_GetTick();

    if(now - lastFrame < 180) return;
    lastFrame = now;

    screen_DrawSleepEyes(z_offset);

    z_offset++;

    if(z_offset > 20)
        z_offset = 0;
}

static void screen_DrawHappyEye(uint8_t x, uint8_t y)
{
    screen_FillRect(x + 8,  y + 10, 4, 4, 1);
    screen_FillRect(x + 12, y + 6,  4, 4, 1);
    screen_FillRect(x + 16, y + 4,  8, 4, 1);
    screen_FillRect(x + 24, y + 6,  4, 4, 1);
    screen_FillRect(x + 28, y + 10, 4, 4, 1);
}

static void screen_DrawHeart(int16_t x, int16_t y)
{
    screen_FillCircle(x + 2, y + 2, 2, 1);
    screen_FillCircle(x + 6, y + 2, 2, 1);

    screen_FillRect(x + 1, y + 2, 7, 3, 1);
    screen_FillRect(x + 2, y + 5, 5, 2, 1);
    screen_FillRect(x + 3, y + 7, 3, 2, 1);
    screen_DrawPixel(x + 4, y + 9, 1);
}

/**
 * @brief Displays a scared face used during edge detection.
 */
void screen_AnimateScaredFace(void)
{
    static uint32_t lastFrame = 0;
    static uint8_t bounce = 0;
    static int8_t dir = 1;
    static uint8_t heartOffset = 0;

    uint32_t now = HAL_GetTick();

    if(now - lastFrame < 120) return;
    lastFrame = now;

    screen_Clear();

    screen_DrawHappyEye(18, 30 + bounce);
    screen_DrawHappyEye(74, 30 + bounce);

    screen_DrawHeart(48, 27 - heartOffset);
    screen_DrawHeart(70, 20 - heartOffset);
    screen_DrawHeart(54, 9  - heartOffset);

    screen_UpdateScreen();

    heartOffset++;

    if(heartOffset > 20)
        heartOffset = 0;

    bounce += dir;

    if(bounce >= 2)
        dir = -1;
    else if(bounce == 0)
        dir = 1;
}
/* =========================
   Extra Big Scared Face
   Same style as normal eyes
   ========================= */

static void screen_DrawExtraBigSameStyleEye(uint8_t x, uint8_t y)
{
    /*
        Same style as your original screen_DrawEye(),
        but pushed close to the max size that fits 128x64.
    */

    screen_FillRect(x + 4, y + 3, 40, 52, 1);
    screen_FillCircle(x + 24, y + 8, 20, 1);
    screen_FillCircle(x + 24, y + 55, 20, 1);

    // Same style black cutouts, scaled up
    screen_FillCircle(x + 14, y + 15, 6, 0);
    screen_FillCircle(x + 31, y + 27, 5, 0);
    screen_FillRect(x + 13, y + 45, 22, 4, 0);
}

static void screen_DrawExtraBigSameStyleEyes(void)
{
    screen_Clear();

    screen_DrawExtraBigSameStyleEye(0, 2);
    screen_DrawExtraBigSameStyleEye(76, 2);

    screen_UpdateScreen();
}

void screen_AnimateScaredFace(void)
{
    static uint32_t lastFrame = 0;

    uint32_t now = HAL_GetTick();

    if(now - lastFrame < 100) return;
    lastFrame = now;

    screen_DrawExtraBigSameStyleEyes();
}
