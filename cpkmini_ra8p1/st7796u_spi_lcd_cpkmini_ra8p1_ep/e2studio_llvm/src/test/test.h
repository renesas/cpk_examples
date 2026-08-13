#ifndef __TEST_H
#define __TEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TEST_EN_LCD
#define TEST_EN_LCD			1
#endif

uint32_t TestAll(void *lcd_device);

#if TEST_EN_LCD
uint32_t TestLCD(void *lcd_device);
#endif

#ifdef __cplusplus
}
#endif

#endif
