#ifndef __LCD_H
#define __LCD_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
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

typedef struct {
    uint16_t high;
    uint16_t width;
    LCD_ColorFormat color_format;
    LCD_ScreenOrientation orientation;
} LCD_Device;

uint32_t LCD_Init(LCD_Device *device);

uint32_t LCD_PortDrawBitmap(LCD_Device *device, uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t *bitmap);
uint32_t LCD_PortDrawPoint(LCD_Device *device, uint16_t x, uint16_t y, uint32_t color);
uint32_t LCD_PortFillRectangle(LCD_Device *device, uint16_t x, uint16_t y, uint16_t length, uint16_t width, uint32_t color);
uint32_t LCD_PortFillScreen(LCD_Device *device, uint32_t color);
uint32_t LCD_PortInit(LCD_Device *device);
uint32_t LCD_PortSetOrientation(LCD_Device *device, LCD_ScreenOrientation orientation);

#ifdef __cplusplus
}
#endif

#endif
