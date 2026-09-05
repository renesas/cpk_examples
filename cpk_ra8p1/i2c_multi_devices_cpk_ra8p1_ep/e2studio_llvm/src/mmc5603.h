#ifndef __MMC5603_H
#define __MMC5603_H

#include <stdint.h>

#define MMC5603_ADDRESS		0x30

#ifdef __cplusplus
extern "C" {
#endif

/* Implement Read and Write functions here. Function prototype:
 * uint32_t MMC5603_ReadMem(uint8_t addr, uint8_t *data, uint16_t length)
 * uint32_t MMC5603_ReadReg(uint8_t reg, uint8_t *val) This *val point a uint8_t val
 * uint32_t MMC5603_WriteReg(uint8_t reg, uint8_t val) */
#include "i2c_comms.h"
#define MMC5603_ReadMem(addr, data, len)	I2C_COMMS_ReadMem(&g_i2c_dev_mmc5603, addr, 8, data, len);
#define MMC5603_ReadReg(reg, val)			I2C_COMMS_ReadReg(&g_i2c_dev_mmc5603, reg, 8, val, 8)
#define MMC5603_WriteReg(reg, val)			I2C_COMMS_WriteReg(&g_i2c_dev_mmc5603, reg, 8, val, 8)

uint32_t MMC5603_GetID(uint8_t *id);
uint32_t MMC5603_GetXYZ(int32_t *x, int32_t *y, int32_t *z);
uint32_t MMC5603_Init(void);

#ifdef __cplusplus
}
#endif

#endif
