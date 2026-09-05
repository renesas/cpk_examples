#include "lcd.h"

#if LCD_DIRVER == LCD_DRIVER_EK79001D

#include "hal_data.h"

#define EK79001D_GLCDC_INSTANCE	g_display0
#define EK79001D_GLCDC_CALLBACK	GLCDC_Callback

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
	R_BSP_PinWrite(BSP_IO_PORT_00_PIN_12, BSP_IO_LEVEL_HIGH);

	device->color_format = COLOR_FORMAT_RGB888;
	device->length = 1024;
	device->orientation = SCREEN_ORIENTATION_HORIZONTAL;
	device->width = 600;

	memset(fb_background, 0x00, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0 * 2);

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
