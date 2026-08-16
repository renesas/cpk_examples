#ifndef __OLED_H
#define __OLED_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	OLED_FONT_6X8,
	OLED_FONT_8X16
} OLED_FontSizeEnum;

typedef struct {
	uint8_t *gram;
	uint32_t (*read)(uint8_t *val, uint16_t len);
	uint32_t (*write)(uint8_t *val, uint16_t len, bool iscmd);
} OLED_Dev;

uint32_t OLED_DrawStr(OLED_Dev *oled, uint8_t x, uint8_t y, char *str, OLED_FontSizeEnum font_size);
uint32_t OLED_Fill(OLED_Dev *oled, uint8_t val);
uint32_t OLED_Init(OLED_Dev *oled);
uint32_t OLED_Off(OLED_Dev *oled);
uint32_t OLED_On(OLED_Dev *oled);
uint32_t OLED_SetPos(OLED_Dev *oled, uint8_t x, uint8_t y);

#ifdef __cplusplus
}
#endif

#endif
