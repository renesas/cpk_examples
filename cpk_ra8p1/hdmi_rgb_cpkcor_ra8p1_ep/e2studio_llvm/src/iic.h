#ifndef __IIC_H
#define __IIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t IIC_Init(void);
uint32_t IIC_Read16bReg8bVal(uint32_t slave, uint16_t reg, uint8_t *val);
uint32_t IIC_Write16bReg8bVal(uint32_t slave, uint16_t reg, uint8_t val);
uint32_t IIC_Write(uint32_t slave, uint8_t *data, uint8_t length);
uint32_t IIC_WriteRead(uint32_t slave, uint8_t *wdata, uint8_t wlen, uint8_t *rdata, uint8_t rlen);

#ifdef __cplusplus
}
#endif

#endif
