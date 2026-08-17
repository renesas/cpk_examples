#ifndef __TEST_H
#define __TEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TEST_EN_AUDIO
#define TEST_EN_AUDIO		1
#endif

#ifndef TEST_EN_HYPER_RAM
#define TEST_EN_HYPER_RAM	0
#endif

#ifndef TEST_EN_LCD
#define TEST_EN_LCD			0
#endif

#ifndef TEST_EN_NOR_FLASH
#define TEST_EN_NOR_FLASH	0
#endif

#ifndef TEST_EN_SDRAM
#define TEST_EN_SDRAM		0
#endif

#if TEST_EN_AUDIO
uint32_t TestAudio(void);
#endif

#if TEST_EN_HYPER_RAM
uint32_t TestHyperRAM(uint32_t start_addr, uint32_t size);
#endif

#if TEST_EN_LCD
uint32_t TestLCD(void *lcd_device);
#endif

#if TEST_EN_SDRAM
uint32_t TestSDRAM(uint32_t start_addr, uint32_t size);
#endif

#ifdef __cplusplus
}
#endif

#endif
