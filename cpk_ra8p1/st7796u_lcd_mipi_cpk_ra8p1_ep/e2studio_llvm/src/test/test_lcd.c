#include "hal_data.h"
#include "test.h"

#if TEST_EN_LCD

#include <stdlib.h>

#include "lcd/lcd.h"
#include "lcd/picture.h"
#include "utils/log.h"

#ifndef TEST_LCD_EN_BACKLIGHT
#define TEST_LCD_EN_BACKLIGHT			0
#endif

#ifndef TEST_LCD_EN_BITMAP_BLOCK
#define TEST_LCD_EN_BITMAP_BLOCK		0
#endif

#ifndef TEST_LCD_EN_COLOR_BAND
#define TEST_LCD_EN_COLOR_BAND          1
#endif

#ifndef TEST_LCD_EN_COLOR_BLOCK
#define TEST_LCD_EN_COLOR_BLOCK			0
#endif

#ifndef TEST_LCD_EN_PICTURE
#define TEST_LCD_EN_PICTURE				0
#endif

#ifndef TEST_LCD_EN_USER_MALLOC
#define TEST_LCD_EN_USER_MALLOC			0
#endif

#if TEST_LCD_EN_USER_MALLOC

#include TEST_LCD_USER_MALLOC_H

#ifndef TEST_LCD_MALLOC
#define TEST_LCD_MALLOC(x)				NULL
#endif

#ifndef TEST_LCD_FREE
#define TEST_LCD_FREE(x)
#endif

#endif

uint32_t TestLCD(void *lcd_device)
{
	uint32_t color[4] = {0};
	LCD_Device *lcd = (LCD_Device *)lcd_device;

	(void)lcd;
	(void)color;

#if TEST_LCD_EN_BACKLIGHT
	LOG_I(__FUNCTION__, "TEST_LCD_EN_BACKLIGHT");
#if LCD_DIRVER == LCD_DRIVER_ST7796U
#if 1
	for (uint8_t i = 0; i < 17; i++) {
		LCD_PortSetBacklight(lcd, i);
		R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
	}
#else
	LCD_PortSetBacklight(lcd, 0);
	R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
#endif
#endif
#endif

#if TEST_LCD_EN_BITMAP_BLOCK
	uint32_t i;
	uint32_t need_size;

	uint16_t *p_bitmap = NULL;

	LOG_I(__FUNCTION__, "TEST_LCD_EN_BITMAP_BLOCK");
	need_size = (lcd->width / 2) * (lcd->length / 2);
#if TEST_LCD_EN_USER_MALLOC
	p_bitmap = TEST_LCD_MALLOC(need_size);
#elif BSP_CFG_RTOS == 1
#elif BSP_CFG_RTOS == 2
#else
	p_bitmap = malloc(need_size);
#endif
	if (p_bitmap != NULL) {
		R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
		for (i = 0; i < need_size; i++) {
			p_bitmap[i] = 0xFFE0;
		}
		LCD_PortDrawBitmap(lcd, 0, 0, lcd->length / 2, lcd->width / 2, (const uint8_t *)p_bitmap);

		R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
		for (i = 0; i < need_size; i++) {
			p_bitmap[i] = 0xFB00;
		}
		LCD_PortDrawBitmap(lcd, lcd->length / 2, 0, lcd->length / 2, lcd->width / 2, (const uint8_t *)p_bitmap);

		R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
		for (i = 0; i < need_size; i++) {
			p_bitmap[i] = 0x07F7;
		}
		LCD_PortDrawBitmap(lcd, 0, lcd->width / 2, lcd->length / 2, lcd->width / 2, (const uint8_t *)p_bitmap);

		R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
		for (i = 0; i < need_size; i++) {
			p_bitmap[i] = 0x701F;
		}
		LCD_PortDrawBitmap(lcd, lcd->length / 2, lcd->width / 2, lcd->length / 2, lcd->width / 2, (const uint8_t *)p_bitmap);
	#if TEST_LCD_EN_USER_MALLOC
		TEST_LCD_FREE(p_bitmap);
	#elif BSP_CFG_RTOS == 1
	#elif BSP_CFG_RTOS == 2
	#else
		free(p_bitmap);
#endif
	}
	else {
		LOG_W(__FUNCTION__, "Can't malloc %u bytes, ignore this case", need_size);
	}
#endif

#if TEST_LCD_EN_COLOR_BAND
	R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
    LOG_I(__FUNCTION__, "TEST_LCD_EN_COLOR_BAND");
    uint32_t colors[] = {0xF800, 0x07E0, 0x001F, 0x0000, 0xFFFF, 0xFFE0, 0xF81F, 0x07FF};
    uint16_t step = (uint16_t)(lcd->width / (sizeof(colors) / sizeof(colors[0])));
    for (uint16_t i = 0; i < (sizeof(colors) / sizeof(colors[0])); i++) {
        LCD_PortFillRectangle(lcd, 0, i * step, lcd->length, step, colors[i]);
    }
#endif

#if TEST_LCD_EN_COLOR_BLOCK
	LOG_I(__FUNCTION__, "TEST_LCD_EN_COLOR_BLOCK");
	switch (lcd->color_format) {
	case COLOR_FORMAT_ARGB1555:
		break;
	case COLOR_FORMAT_ARGB4444:
		break;
	case COLOR_FORMAT_ARGB8888:
	case COLOR_FORMAT_RGB888:
		color[0] = 0x00FF0000;
		color[1] = 0x0000FF00;
		color[2] = 0x000000FF;
		color[3] = 0x00FBAD03;
		break;
	case COLOR_FORMAT_RGB565:
		color[0] = 0xF800;
		color[1] = 0x07E0;
		color[2] = 0x001F;
		color[3] = 0xFDA0;
		break;
	case COLOR_FORMAT_RGB666:
		break;
	default:
		break;
	}
	R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
	LCD_PortFillRectangle(lcd, 0, 0, lcd->length / 2, lcd->width / 2, color[0]);
	R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
	LCD_PortFillRectangle(lcd, lcd->length / 2, 0, lcd->length / 2, lcd->width / 2, color[1]);
	R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
	LCD_PortFillRectangle(lcd, 0, lcd->width / 2, lcd->length / 2, lcd->width / 2, color[2]);
	R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
	LCD_PortFillRectangle(lcd, lcd->length / 2, lcd->width / 2, lcd->length / 2, lcd->width / 2, color[3]);
#endif

#if TEST_LCD_EN_PICTURE
	LOG_I(__FUNCTION__, "TEST_LCD_EN_PICTURE");
	LOG_I(__FUNCTION__, "You need to ensure that the orientation and format of the image are correct");
	R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
	LCD_PortDrawBitmap(lcd, 0, 0, (uint16_t)g_picture_222x480_01.width, (uint16_t)g_picture_222x480_01.height, (const uint8_t *)g_picture_222x480_01.pixel_data);
	R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_SECONDS);
    LCD_PortDrawBitmap(lcd, 0, 0, (uint16_t)g_picture_222x480_02.width, (uint16_t)g_picture_222x480_02.height, (const uint8_t *)g_picture_222x480_02.pixel_data);
	R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_SECONDS);
#endif

	return 0;
}

#endif
