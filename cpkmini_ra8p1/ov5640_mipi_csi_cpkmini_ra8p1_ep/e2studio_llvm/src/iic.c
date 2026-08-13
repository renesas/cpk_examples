#include "hal_data.h"
#include "iic.h"

#define IIC0_MASTER		g_i2c_master0
#define IIC0_MASTER_CB	IIC0_Master_Callback
#define IIC0_TIMEOUT	200

#ifndef __IIC_DEBUG
#define __IIC_DEBUG 1
#endif

#if __IIC_DEBUG

#include "utils/log.h"

#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#endif

#define IIC_WAIT_TX_DONE(flag)	do { \
									uint32_t timeout = IIC0_TIMEOUT; \
									while (flag.bit.tx_done == 0) { \
										UNLIKE_RETURN(flag.bit.abort, 0, "Abort"); \
										R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS); \
										timeout--; \
										LIKE_RETURN(timeout, 0, "Timeout"); \
									} \
								} while(0)
#define IIC_WAIT_RX_DONE(flag)	do { \
									uint32_t timeout = IIC0_TIMEOUT; \
									while (flag.bit.rx_done == 0) { \
										UNLIKE_RETURN(flag.bit.abort, 0, "Abort"); \
										R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS); \
										timeout--; \
										LIKE_RETURN(timeout, 0, "Timeout"); \
									} \
								} while(0)

union IIC_Flag {
	uint8_t byte;
	struct {
		uint8_t abort : 1;
		uint8_t rx_done : 1;
		uint8_t tx_done : 1;
		uint8_t : 5;
	} bit;
};

static union IIC_Flag s_iic0_master_flag;

uint32_t IIC_Init(void)
{
	fsp_err_t err;

	err = R_IIC_MASTER_Open(IIC0_MASTER.p_ctrl, IIC0_MASTER.p_cfg);
#if __IIC_DEBUG
	LOG_D(__FUNCTION__, "IIC Open: %u", err);
#endif

	return err;
}

uint32_t IIC_Read16bReg8bVal(uint32_t slave, uint16_t reg, uint8_t *val)
{
	fsp_err_t err;
	uint8_t wcache[2];

	R_IIC_MASTER_SlaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	s_iic0_master_flag.byte = 0;
	wcache[0] = (uint8_t)((reg >> 8) & 0xFF);
	wcache[1] = (uint8_t)(reg & 0xFF);
	err = R_IIC_MASTER_Write(IIC0_MASTER.p_ctrl, wcache, 2, true);
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Write failed: 0x%X", err);
	IIC_WAIT_TX_DONE(s_iic0_master_flag);
	s_iic0_master_flag.byte = 0;
	err = R_IIC_MASTER_Read(IIC0_MASTER.p_ctrl, val, 1, false);
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Read failed: 0x%X", err);
	IIC_WAIT_RX_DONE(s_iic0_master_flag);

	return err;
}

uint32_t IIC_Write16bReg8bVal(uint32_t slave, uint16_t reg, uint8_t val)
{
	fsp_err_t err;
	uint8_t wcache[3];

	R_IIC_MASTER_SlaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	s_iic0_master_flag.byte = 0;
	wcache[0] = (uint8_t)((reg >> 8) & 0xFF);
	wcache[1] = (uint8_t)(reg & 0xFF);
	wcache[2] = val;
	err = R_IIC_MASTER_Write(IIC0_MASTER.p_ctrl, wcache, 3, false);
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Write failed: 0x%X", err);
	IIC_WAIT_TX_DONE(s_iic0_master_flag);

	return err;
}

uint32_t IIC_Write(uint32_t slave, uint8_t *data, uint8_t length)
{
	fsp_err_t err;

	iic_master_instance_ctrl_t *p_ctrl = (iic_master_instance_ctrl_t *)IIC0_MASTER.p_ctrl;
	if (p_ctrl->slave != slave) {
		R_IIC_MASTER_SlaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	s_iic0_master_flag.byte = 0;
	err = R_IIC_MASTER_Write(IIC0_MASTER.p_ctrl, data, length, true);
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Write failed: 0x%X", err);
	IIC_WAIT_TX_DONE(s_iic0_master_flag);

	return err;
}

uint32_t IIC_WriteRead(uint32_t slave, uint8_t *wdata, uint8_t wlen, uint8_t *rdata, uint8_t rlen)
{
	fsp_err_t err;

	iic_master_instance_ctrl_t *p_ctrl = (iic_master_instance_ctrl_t *)IIC0_MASTER.p_ctrl;
	if (p_ctrl->slave != slave) {
		R_IIC_MASTER_SlaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	s_iic0_master_flag.byte = 0;
	err = R_IIC_MASTER_Write(IIC0_MASTER.p_ctrl, wdata, wlen, true);
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Write failed: 0x%X", err);
	IIC_WAIT_TX_DONE(s_iic0_master_flag);
	s_iic0_master_flag.byte = 0;
	err = R_IIC_MASTER_Read(IIC0_MASTER.p_ctrl, rdata, rlen, false);
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Read failed: 0x%X", err);
	IIC_WAIT_RX_DONE(s_iic0_master_flag);

	return err;
}

void IIC0_MASTER_CB(i2c_master_callback_args_t *p_args)
{
	switch (p_args->event) {
	case I2C_MASTER_EVENT_ABORTED:
		s_iic0_master_flag.bit.abort = 1;
		break;
	case I2C_MASTER_EVENT_RX_COMPLETE:
		s_iic0_master_flag.bit.rx_done = 1;
		break;
	case I2C_MASTER_EVENT_TX_COMPLETE:
		s_iic0_master_flag.bit.tx_done = 1;
		break;
	case I2C_MASTER_EVENT_START:
	case I2C_MASTER_EVENT_BYTE_ACK:
	default:
		break;
	}
}
