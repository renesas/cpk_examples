#ifndef __I3C_H
#define __I3C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t I3C_CheckHotjoin(void);
uint32_t I3C_Init(void);
uint32_t I3C_ReadMemory(uint8_t reg, uint8_t *data, uint32_t length);
uint32_t I3C_WriteMemory(uint8_t reg, const uint8_t *data, uint32_t length);
uint8_t I3C_GetDynamicAddress(void);

#ifdef __cplusplus
}
#endif

#endif
