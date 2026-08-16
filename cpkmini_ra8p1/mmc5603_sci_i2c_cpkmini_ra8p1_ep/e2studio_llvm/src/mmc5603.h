#ifndef __MMC5603_H
#define __MMC5603_H

#include <stdint.h>

#define MMC5603_ADDRESS		0x30

#ifdef __cplusplus
extern "C" {
#endif

uint32_t MMC5603_GetID(uint8_t *id);
uint32_t MMC5603_GetXYZ(int32_t *x, int32_t *y, int32_t *z);
uint32_t MMC5603_Init(void);

#ifdef __cplusplus
}
#endif

#endif
