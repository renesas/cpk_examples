#include "lcd.h"

#if LCD_DIRVER == LCD_DRIVER_EK79001D

#include "hal_data.h"
#include "utils/util.h"

#define EK79001D_GLCDC_INSTANCE	g_lcdc_ek79001
#define EK79001D_GLCDC_CALLBACK	GLCDC_Callback
#define EK79001D_GLCDC_CFG		UTIL_CONCAT(EK79001D_GLCDC_INSTANCE, _cfg)
#define EK79001D_PIN_BACKLIGHT	BSP_IO_PORT_00_PIN_12
#define EK79001D_PIN_RESET		BSP_IO_PORT_00_PIN_13

#ifndef __EK79001D_DEBUG
#define __EK79001D_DEBUG		1
#endif

#if __EK79001D_DEBUG
#include "utils/log.h"
#endif

/*
static const ioport_pin_cfg_t sc_lcd_pin_cfg_data[] = {
	{.pin = BSP_IO_PORT_05_PIN_15, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_09_PIN_14, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_09_PIN_15, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_09_PIN_03, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_09_PIN_02, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_09_PIN_10, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_09_PIN_11, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_09_PIN_12, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_09_PIN_13, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_09_PIN_04, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_02_PIN_07, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_11_PIN_07, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_11_PIN_06, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_11_PIN_05, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_11_PIN_01, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_11_PIN_04, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_11_PIN_03, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_11_PIN_02, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_11_PIN_00, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_07_PIN_07, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_07_PIN_11, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_07_PIN_12, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_07_PIN_13, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_07_PIN_14, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_07_PIN_15, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_05_PIN_14, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_08_PIN_06, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_08_PIN_05, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_08_PIN_07, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	{.pin = BSP_IO_PORT_05_PIN_13, .pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_HIGH | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_CFG_PULLUP_ENABLE | (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS},
	// {.pin = EK79001D_PIN_BACKLIGHT, .pin_cfg = (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_LOW},
	// {.pin = EK79001D_PIN_RESET, .pin_cfg = (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_LOW},
};

static const ioport_cfg_t sc_lcd_pin_cfg = {
	.number_of_pins = sizeof(sc_lcd_pin_cfg_data) / sizeof(sc_lcd_pin_cfg_data[0]),
	.p_pin_cfg_data = &sc_lcd_pin_cfg_data[0],
	.p_extend = NULL
};

static ioport_instance_ctrl_t s_lcd_pin_ctrl;
*/

static volatile uint8_t s_glcdc_vsync;

uint32_t LCD_PortFillRectangle(LCD_Device *device, uint16_t x, uint16_t y, uint16_t length, uint16_t width, uint32_t color)
{
	uint16_t i, j;

	glcdc_instance_ctrl_t *pctrl = (glcdc_instance_ctrl_t *)EK79001D_GLCDC_INSTANCE.p_ctrl;
	uint32_t *p32_bg = pctrl->p_cfg->input[0].p_base;

	if ((x >= device->length) || (y >= device->width)) {
		return FSP_ERR_ASSERTION;
	}

	if ((x + length) > device->length) {
		length = device->length - x;
	}
	if ((y + width) > device->width) {
		width = device->width - y;
	}

#if __EK79001D_DEBUG
	LOG_D(__FUNCTION__, "color: 0x%X", color);
#endif

	p32_bg = &p32_bg[y * device->length];
	for (i = 0; i < width; i++) {
		for (j = x; j < (x + length); j++) {
			p32_bg[j] = color;
		}
		p32_bg = &p32_bg[device->length];
	}

	return FSP_SUCCESS;
}

uint32_t LCD_PortInit(LCD_Device *device)
{
	R_GLCDC_Open(EK79001D_GLCDC_INSTANCE.p_ctrl, EK79001D_GLCDC_INSTANCE.p_cfg);
	R_GLCDC_Start(EK79001D_GLCDC_INSTANCE.p_ctrl);
	R_BSP_PinAccessEnable();
	R_BSP_PinWrite(EK79001D_PIN_BACKLIGHT, BSP_IO_LEVEL_HIGH);
	R_BSP_PinWrite(EK79001D_PIN_RESET, BSP_IO_LEVEL_HIGH);
	R_BSP_PinAccessDisable();

	device->color_format = COLOR_FORMAT_RGB888;
	device->length = 1024;
	device->orientation = SCREEN_ORIENTATION_HORIZONTAL;
	device->width = 600;

	memset(EK79001D_GLCDC_CFG.input[0].p_base, 0x00, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0 * 2);

	return FSP_SUCCESS;
}

void EK79001D_GLCDC_CALLBACK(display_callback_args_t * p_args)
{
	switch (p_args->event) {
	case DISPLAY_EVENT_GR1_UNDERFLOW:
		break;
	case DISPLAY_EVENT_GR2_UNDERFLOW:
		break;
	case DISPLAY_EVENT_LINE_DETECTION:
		s_glcdc_vsync = 1;
		break;
	case DISPLAY_EVENT_FRAME_END:
		break;
	default:
		break;
	}
}

#endif /* #if LCD_DIRVER == LCD_DRIVER_EK79001D */
