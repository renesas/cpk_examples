#ifndef __LCD_H
#define __LCD_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_DRIVER_UNSELECT		0
#define LCD_DRIVER_EK79001D		1
#define LCD_DRIVER_ILI9881C		2
#define LCD_DRIVER_ST7796U		3
#define LCD_DRIVER_TFP410		4

#ifndef LCD_DIRVER
#define LCD_DIRVER				LCD_DRIVER_ST7796U
#endif

typedef enum {
    COLOR_FORMAT_ARGB1555,
    COLOR_FORMAT_ARGB4444,
    COLOR_FORMAT_ARGB8888,
    COLOR_FORMAT_RGB565,
    COLOR_FORMAT_RGB666,
    COLOR_FORMAT_RGB888,
} LCD_ColorFormat;

typedef enum {
    SCREEN_ORIENTATION_HORIZONTAL,
    SCREEN_ORIENTATION_HORIZONTAL_FLIP,
    SCREEN_ORIENTATION_VERTICAL,
    SCREEN_ORIENTATION_VERTICAL_FLIP
} LCD_ScreenOrientation;

/* When using GLCDC, there is an integer multiple limit on the size of the graphics memory of the GLCDC.
 * The variables "length_dummy" and "width_dummy" record the difference between the GLCDC video memory size and the physical size of the screen. */
typedef struct {
    uint16_t length;
    uint16_t length_dummy;
    uint16_t width;
    uint16_t width_dummy;
    LCD_ColorFormat color_format;
    LCD_ScreenOrientation orientation;
} LCD_Device;

uint32_t LCD_Init(LCD_Device *device);

uint32_t LCD_PortDrawBitmap(LCD_Device *device, uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t *bitmap);
uint32_t LCD_PortDrawPoint(LCD_Device *device, uint16_t x, uint16_t y, uint32_t color);
uint32_t LCD_PortFillRectangle(LCD_Device *device, uint16_t x, uint16_t y, uint16_t length, uint16_t width, uint32_t color);
uint32_t LCD_PortFillScreen(LCD_Device *device, uint32_t color);
uint32_t LCD_PortInit(LCD_Device *device);
uint32_t LCD_PortSetBacklight(LCD_Device *device, uint8_t bl);
uint32_t LCD_PortSetDisplayOff(LCD_Device *device);
uint32_t LCD_PortSetDisplayOn(LCD_Device *device);
uint32_t LCD_PortSetInversionOff(LCD_Device *device);
uint32_t LCD_PortSetInversionOn(LCD_Device *device);
uint32_t LCD_PortSetOrientation(LCD_Device *device, LCD_ScreenOrientation orientation);

#ifdef __cplusplus
}
#endif

#endif
