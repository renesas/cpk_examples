#ifndef __TEST_H
#define __TEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TEST_EN_AT24C256
#define TEST_EN_AT24C256	1
#endif

#ifndef TEST_EN_LCD
#define TEST_EN_LCD			0
#endif

#if TEST_EN_AT24C256
uint32_t TestAT24C256(void);
#endif

#if TEST_EN_LCD
void TestLCD(void *lcd_device);
#endif

#ifdef __cplusplus
}
#endif

#endif
