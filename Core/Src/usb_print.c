#include "usb_print.h"
#include "main.h"
#include "usbd_cdc_if.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static char usb_print_buf[256];

void USB_Print(const char *text)
{
#if USB_DEBUG_ENABLED
    uint32_t start = HAL_GetTick();
    uint8_t result;

    if (text == NULL) return;

    do {
        result = CDC_Transmit_FS((uint8_t *)text, strlen(text));
        if (result == USBD_OK) {
            HAL_Delay(20);
            return;
        }
        HAL_Delay(1);
    } while ((HAL_GetTick() - start) < 1000);
#else
    (void)text;
#endif
}

void USB_Printf(const char *fmt, ...)
{
#if USB_DEBUG_ENABLED
    va_list args;

    if (fmt == NULL) return;

    va_start(args, fmt);
    vsnprintf(usb_print_buf, sizeof(usb_print_buf), fmt, args);
    va_end(args);

    USB_Print(usb_print_buf);
#else
    (void)fmt;
#endif
}
