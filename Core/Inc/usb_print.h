/*
 * usb_print.h
 *
 * USB CDC printing helpers.
 */

#ifndef INC_USB_PRINT_H_
#define INC_USB_PRINT_H_


#include <stdint.h>

/*
 * Set to 1 for USB serial debug.
 * Set to 0 for normal robot mode.
 */
#define USB_DEBUG_ENABLED 0

void USB_Print(const char *text);
void USB_Printf(const char *fmt, ...);


#endif /* INC_USB_PRINT_H_ */
