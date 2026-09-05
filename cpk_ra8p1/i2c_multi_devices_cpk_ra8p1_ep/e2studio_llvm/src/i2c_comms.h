#ifndef __I2C_COMMS_H
#define __I2C_COMMS_H

#include "hal_data.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t I2C_COMMS_Init(const rm_comms_instance_t *i2c);
uint32_t I2C_COMMS_ReadMem(const rm_comms_instance_t *i2c, uint16_t mem_addr, uint8_t addr_width, uint8_t *rdata, uint16_t rlen);
uint32_t I2C_COMMS_ReadReg(const rm_comms_instance_t *i2c, uint16_t reg_addr, uint8_t addr_width, uint8_t *val, uint8_t val_width);
uint32_t I2C_COMMS_Write(const rm_comms_instance_t *i2c, uint8_t *data, uint16_t length);
uint32_t I2C_COMMS_WriteMem(const rm_comms_instance_t *i2c, uint16_t mem_addr, uint8_t addr_width, const uint8_t *wdata, uint16_t wlen);
uint32_t I2C_COMMS_WriteRead(const rm_comms_instance_t *i2c, uint8_t *wdata, uint16_t wlen, uint8_t *rdata, uint16_t rlen);
uint32_t I2C_COMMS_WriteReg(const rm_comms_instance_t *i2c, uint16_t reg_addr, uint8_t addr_width, uint16_t val, uint8_t val_width);

uint32_t I2C_COMMS_SCCB_ReadReg(const rm_comms_instance_t *i2c, uint16_t reg, uint8_t reg_width, uint8_t *val);
uint32_t I2C_COMMS_SCCB_WriteReg(const rm_comms_instance_t *i2c, uint16_t reg, uint8_t reg_width, uint8_t val);

#ifdef __cplusplus
}
#endif

#endif
