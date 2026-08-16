#include "lcd.h"

#if LCD_DIRVER == LCD_DRIVER_EK79007

#include "hal_data.h"
#include "utils/util.h"

#define EK79007_GLCDC_INSTANCE		g_lcdc_ek79007
#define EK79007_GLCDC_CALLBACK		GLCDC_Callback
#define EK79007_GLCDC_CFG			UTIL_CONCAT(g_lcdc_ek79007, _cfg)
#define EK79007_MIPI_INSTANCE		g_mipi_dsi_ek79007
#define EK79007_MIPI_CALLBACK		MIPI_DSI_Callback
#define EK79007_PIN_BACKLIGHT		BSP_IO_PORT_00_PIN_12
#define EK79007_PIN_RESET			BSP_IO_PORT_00_PIN_13

#ifndef __EK79007_DEBUG
#define __EK79007_DEBUG		0
#endif

struct lcd_mipi_table_setting {
    unsigned char size;
    unsigned char buffer[10];
    mipi_cmd_id_t cmd_id;
    mipi_dsi_cmd_flag_t flags;
};

static const struct lcd_mipi_table_setting sc_lcd_init_table[] = {
	{2, {0xB2, 0x10}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
	{2, {0x80, 0xAC}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
	{2, {0x81, 0xB8}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
	{2, {0x82, 0x09}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
	{2, {0x83, 0x78}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
	{2, {0x84, 0x7F}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
	{2, {0x85, 0xBB}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
	{2, {0x86, 0x70}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER}
};

static volatile uint8_t s_mipi_event_seq0;
static volatile uint8_t s_glcdc_vsync;

uint32_t LCD_PortFillRectangle(LCD_Device *device, uint16_t x, uint16_t y, uint16_t length, uint16_t width, uint32_t color)
{
	if ((length == 0) || (width == 0)) {
        return 0;
    }
	if ((x >= device->length) || (y >= device->width)) {
		return 1;
	}
	if ((x + length) > device->length) {
		length = device->length - x;
	}
	if ((y + width) > device->width) {
		width = device->width - y;
	}

	uint16_t i, j;
	uint16_t *p16_bg = (uint16_t *)&fb_background[0][0];
	uint32_t *p32_bg = (uint32_t *)&fb_background[0][0];

	switch (device->color_format) {
	case COLOR_FORMAT_ARGB8888:
	case COLOR_FORMAT_RGB888:
		p32_bg = &p32_bg[y * device->length];
		for (i = 0; i < width; i++) {
			for (j = x; j < (x + length); j++) {
				p32_bg[j] = color;
			}
			p32_bg = &p32_bg[device->length];
		}
		break;
	case COLOR_FORMAT_ARGB1555:
	case COLOR_FORMAT_ARGB4444:
	case COLOR_FORMAT_RGB565:
		p16_bg = &p16_bg[y * device->length];
		for (i = 0; i < width; i++) {
			for (j = x; j < (x + length); j++) {
				p16_bg[j] = (uint16_t)color;
			}
			p16_bg = &p16_bg[device->length];
		}
		break;
	default:
		break;
	}

	return 0;
}

uint32_t LCD_PortInit(LCD_Device *device)
{
	uint32_t i;
	mipi_dsi_cmd_t mipi_cmd;

	R_BSP_PinAccessEnable();
	R_BSP_PinWrite(EK79007_PIN_RESET, BSP_IO_LEVEL_HIGH);
	R_BSP_PinWrite(EK79007_PIN_BACKLIGHT, BSP_IO_LEVEL_HIGH);
	R_BSP_PinAccessDisable();
	R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);

	R_GLCDC_Open(EK79007_GLCDC_INSTANCE.p_ctrl, EK79007_GLCDC_INSTANCE.p_cfg);

	mipi_cmd.channel = 0;
    mipi_cmd.p_rx_buffer = NULL;
    for (i = 0; i < sizeof(sc_lcd_init_table) / sizeof(sc_lcd_init_table[0]); i++) {
        mipi_cmd.cmd_id = sc_lcd_init_table[i].cmd_id;
        mipi_cmd.flags = sc_lcd_init_table[i].flags;
        mipi_cmd.p_tx_buffer = sc_lcd_init_table[i].buffer;
        mipi_cmd.tx_len = sc_lcd_init_table[i].size;
        s_mipi_event_seq0 = 0;
        R_MIPI_DSI_Command(EK79007_MIPI_INSTANCE.p_ctrl, &mipi_cmd);
        while (s_mipi_event_seq0 == 0) {
            __NOP();
        }
        R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MILLISECONDS);
    }
    R_GLCDC_Start(EK79007_GLCDC_INSTANCE.p_ctrl);

    device->color_format = COLOR_FORMAT_RGB565;
    device->width = DISPLAY_VSIZE_INPUT0;
    device->length = DISPLAY_HSIZE_INPUT0;
    device->orientation = SCREEN_ORIENTATION_HORIZONTAL;

    memset(EK79007_GLCDC_CFG.input[0].p_base, 0x00, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0 * 2);

	return 0;
}

void EK79007_GLCDC_CALLBACK(display_callback_args_t * p_args)
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

void EK79007_MIPI_CALLBACK(mipi_dsi_callback_args_t *p_args)
{
    switch (p_args->event) {
    case MIPI_DSI_EVENT_SEQUENCE_0:
        s_mipi_event_seq0 = 1;
        break;
    default:
        break;
    }
}

#endif
