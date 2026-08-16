#include "oled.h"

#ifndef __OLED_DEBUG
#define __OLED_DEBUG	1
#endif

#if __OLED_DEBUG
#include "utils/log.h"
#define TAG 						__FUNCTION__
#define OLED_LOGD(msg, ...)			LOG_D(TAG, msg, ##__VA_ARGS__)
#define OLED_LOGI(msg, ...)			LOG_I(TAG, msg, ##__VA_ARGS__)
#define OLED_LOGW(msg, ...)			LOG_W(TAG, msg, ##__VA_ARGS__)
#define OLED_LOGE(msg, ...)			LOG_E(TAG, msg, ##__VA_ARGS__)
#else
#define OLED_LOGD(msg, ...)
#define OLED_LOGI(msg, ...)
#define OLED_LOGW(msg, ...)
#define OLED_LOGE(msg, ...)
#endif

#define OLED_IIC_WRITE_CMD(val)		{ wcache[0] = val; if (oled->write(wcache, 1, true)) { OLED_LOGE("Write cmd 0x%" PRIX8 " failed", val); return 2; } }
#define OLED_IIC_WRITE_DATA(val)	{ wcache[0] = val; if (oled->write(wcache, 1, false)) { OLED_LOGE("Write data 0x%" PRIx8 " failed", val); return 2; } }

extern const char g_oled_font_6x8[];
extern const char g_oled_font_8x16[];

uint32_t OLED_DrawStr(OLED_Dev *oled, uint8_t x, uint8_t y, char *str, OLED_FontSizeEnum font_size)
{
	char c;
	uint8_t i, j;
	uint8_t wcache[1];

	switch(font_size) {
	case OLED_FONT_6X8:
		for (i = 0; str[i]; i++) {
			c = str[i] - 32;
			if (x > 126) {
				x = 0;
				y++;
			}
			OLED_SetPos(oled, x, y);
			for (j = 0; j < 6; j++) {
				OLED_IIC_WRITE_DATA(g_oled_font_6x8[c * 6 + j]);
			}
			x += 6;
		}
		break;
	case OLED_FONT_8X16:
		for (i = 0; str[i]; i++) {
			c = str[i] - 32;
			if (x > 120) {
				x = 0;
				y += 2;
			}
			OLED_SetPos(oled, x, y);
			for (j = 0; j < 8; j++) {
				OLED_IIC_WRITE_DATA(g_oled_font_8x16[c * 16 + j]);
			}
			OLED_SetPos(oled, x, y + 1);
			for (j = 0; j < 8; j++) {
				OLED_IIC_WRITE_DATA(g_oled_font_8x16[c * 16 + j + 8]);
			}
			x += 8;
		}
		break;
	default:
		break;
	}

	return 0;
}

uint32_t OLED_Fill(OLED_Dev *oled, uint8_t val)
{
	uint8_t i, j;
	uint8_t wcache[1];

	for (i = 0; i < 8; i++) {
		OLED_IIC_WRITE_CMD(0xB0 + i);
		OLED_IIC_WRITE_CMD(0x00);
		OLED_IIC_WRITE_CMD(0x10);
		for (j = 0; j < 128; j++) {
			OLED_IIC_WRITE_DATA(val);
		}
	}

	return 0;
}

uint32_t OLED_Init(OLED_Dev *oled)
{
	uint8_t wcache[1];

	if (oled->write == NULL) {
		return 1;
	}

	OLED_IIC_WRITE_CMD(0xAE);
	OLED_IIC_WRITE_CMD(0x20);
	OLED_IIC_WRITE_CMD(0x10);
	OLED_IIC_WRITE_CMD(0xB0);
	OLED_IIC_WRITE_CMD(0xC8);
	OLED_IIC_WRITE_CMD(0x00);
	OLED_IIC_WRITE_CMD(0x10);
	OLED_IIC_WRITE_CMD(0x40);
	OLED_IIC_WRITE_CMD(0x81);
	OLED_IIC_WRITE_CMD(0xFF);
	OLED_IIC_WRITE_CMD(0xA1);
	OLED_IIC_WRITE_CMD(0xA6);
	OLED_IIC_WRITE_CMD(0xA8);
	OLED_IIC_WRITE_CMD(0x3F);
	OLED_IIC_WRITE_CMD(0xA4);
	OLED_IIC_WRITE_CMD(0xD3);
	OLED_IIC_WRITE_CMD(0x00);
	OLED_IIC_WRITE_CMD(0xD5);
	OLED_IIC_WRITE_CMD(0xF0);
	OLED_IIC_WRITE_CMD(0xD9);
	OLED_IIC_WRITE_CMD(0x22);
	OLED_IIC_WRITE_CMD(0xDA);
	OLED_IIC_WRITE_CMD(0x12);
	OLED_IIC_WRITE_CMD(0xDB);
	OLED_IIC_WRITE_CMD(0x20);
	OLED_IIC_WRITE_CMD(0x8D);
	OLED_IIC_WRITE_CMD(0x14);
	OLED_IIC_WRITE_CMD(0xAF);

	return 0;
}

uint32_t OLED_Off(OLED_Dev *oled)
{
	uint8_t wcache[1];

	OLED_IIC_WRITE_CMD(0x8D);
	OLED_IIC_WRITE_CMD(0x10);
	OLED_IIC_WRITE_CMD(0xAE);

	return 0;
}

uint32_t OLED_On(OLED_Dev *oled)
{
	uint8_t wcache[1];

	OLED_IIC_WRITE_CMD(0x8D);
	OLED_IIC_WRITE_CMD(0x14);
	OLED_IIC_WRITE_CMD(0xAF);

	return 0;
}

uint32_t OLED_SetPos(OLED_Dev *oled, uint8_t x, uint8_t y)
{
	uint8_t wcache[1];

	OLED_IIC_WRITE_CMD(0xB0 + y);
	OLED_IIC_WRITE_CMD(((x & 0xF0) >> 4) | 0x10);
	OLED_IIC_WRITE_CMD((x & 0x0F) | 0x01);

	return 0;
}
