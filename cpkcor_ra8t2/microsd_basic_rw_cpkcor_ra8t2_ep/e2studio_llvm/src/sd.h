#ifndef __SD_H
#define __SD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t SD_Init(void);
uint32_t SD_InitMedia(void);
uint32_t SD_IsInsert(void);
uint32_t SD_IsTransDone(void);
uint32_t SD_Read(uint8_t *data, uint32_t block_addr, uint32_t size);
uint32_t SD_WaitTrans(void);
uint32_t SD_Write(uint8_t const *src, uint32_t block_addr, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
