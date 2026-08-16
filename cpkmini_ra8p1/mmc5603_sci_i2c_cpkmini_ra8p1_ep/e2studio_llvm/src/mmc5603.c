#include "hal_data.h"
#include "iic.h"
#include "mmc5603.h"

#define REG_XOUT_0		0x00
#define REG_XOUT_1		0x01
#define REG_YOUT_0		0x02
#define REG_YOUT_1		0x03
#define REG_ZOUT_0		0x04
#define REG_ZOUT_1		0x05
#define REG_XOUT_2		0x06
#define REG_YOUT_2		0x07
#define REG_ZOUT_2		0x08
#define REG_STATUS		0x18
#define REG_ODR			0x1A
#define REG_CONTROL_0	0x1B
#define REG_CONTROL_1	0x1C
#define REG_CONTROL_2	0x1D
#define REG_ST_X_TH		0x1E
#define REG_ST_Y_TH		0x1F
#define REG_ST_Z_TH		0x20
#define REG_ST_X		0x27
#define REG_ST_Y		0x28
#define REG_ST_Z		0x29
#define REG_PRODUCT_ID	0x39

#ifndef __MMC5603_DEBUG
#define __MMC5603_DEBUG	1
#endif

#if __MMC5603_DEBUG
#include "utils/log.h"
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#endif

#define IIC_READ(reg, val)			IIC_ReadReg(MMC5603_ADDRESS, reg, 8, val, 8)
#define IIC_READ_CHECK(reg, val)	{ err = IIC_READ(reg, val); UNLIKE_RETURN(err, 0, "Read failed: 0x%u", err); }
#define IIC_WRITE(reg, val)			IIC_WriteReg(MMC5603_ADDRESS, reg, 8, val, 8)
#define IIC_WRITE_CHECK(reg, val)	{ err = IIC_WRITE(reg, val); UNLIKE_RETURN(err, 0, "Write failed: 0x%u", err); }

uint32_t MMC5603_GetID(uint8_t *id)
{
	uint8_t _id;
	uint32_t err;

	IIC_READ_CHECK(REG_PRODUCT_ID, &_id);
	*id = _id;

	return err;
}

uint32_t MMC5603_GetXYZ(int32_t *x, int32_t *y, int32_t *z)
{
	uint32_t err;
	uint8_t rcache[9];

	IIC_READ_CHECK(REG_STATUS, rcache);
	if ((rcache[0] & 0x40) == 0) {
	#if __MMC5603_DEBUG
		LOG_E(__FUNCTION__, "Not ready");
	#endif
		return FSP_ERR_ABORTED;
	}
	err = IIC_ReadMemory(MMC5603_ADDRESS, 0x00, 8, rcache, 9);
	UNLIKE_RETURN(err, 0, "Read failed: %u", err);

	*x = (rcache[0] << 12) | (rcache[1] << 4) | (rcache[6] >> 4);
	*x = *x - 524288;
	*y = (rcache[2] << 12) | (rcache[3] << 4) | (rcache[7] >> 4);
	*y = *y - 524288;
	*z = (rcache[4] << 12) | (rcache[5] << 4) | (rcache[8] >> 4);
	*z = *z - 524288;

	return err;
}

uint32_t MMC5603_Init(void)
{
	uint8_t id;
	uint32_t err;

	err = MMC5603_GetID(&id);
	UNLIKE_RETURN(err, 0, "GetID failed");
	UNLIKE_RETURN(id, 0x10, "ID error. Expect: 0x10, Read: 0x%X", id);

	/* Soft reset */
	IIC_WRITE_CHECK(REG_CONTROL_1, 0x80);
	R_BSP_SoftwareDelay(25, BSP_DELAY_UNITS_MILLISECONDS);

	/* Enter continue mode */
	IIC_WRITE_CHECK(REG_ODR, 0x64);
	IIC_WRITE_CHECK(REG_CONTROL_0, 0xE0);
	IIC_WRITE_CHECK(REG_CONTROL_1, 0x02);
	R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
	IIC_WRITE_CHECK(REG_CONTROL_2, 0x19);

	return 0;
}
