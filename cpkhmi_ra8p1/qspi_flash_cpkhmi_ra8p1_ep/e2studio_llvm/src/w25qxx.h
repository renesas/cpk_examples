#ifndef __W25QXX_H
#define __W25QXX_H

#include <stdint.h>

#define W25QXX_MAP_START_ADDR	0x80000000
#define W25QXX_MAP_END_ADDR		0x90000000
#define W25QXX_SIZE				(1024 * 1024 * 32)
#define W25QXX_SECTOR_SIZE		(1024 * 4)
#define W25QXX_BLOCK_SIZE		(1024 * 64)

#ifdef __cplusplus
extern "C" {
#endif

uint32_t W25QXX_EraseChip(void);
uint32_t W25QXX_EraseSector(uint32_t sector);
uint32_t W25QXX_GetID(uint8_t *manufacturer_id, uint8_t *device_id, uint64_t *unique_id);
uint32_t W25QXX_Init(void);
uint32_t W25QXX_WaitOperation(uint32_t timeout);
uint32_t W25QXX_Write(uint8_t * const src, uint8_t * const dest, uint32_t length, uint8_t erase);
uint32_t W25QXX_WriteEnable(uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif
