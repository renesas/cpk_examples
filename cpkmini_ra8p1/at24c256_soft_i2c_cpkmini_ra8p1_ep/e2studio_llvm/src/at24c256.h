#ifndef __AT24C256_H
#define __AT24C256_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t AT24C256_Init(void);
uint32_t AT24C256_Read(uint16_t addr, uint8_t *rdata, uint16_t length);
uint32_t AT24C256_Write(uint16_t addr, uint8_t *wdata, uint16_t length);
uint32_t AT24C256_Fill(uint16_t addr, uint8_t value, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif
