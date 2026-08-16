#include "hal_data.h"
#include "iic.h"
#include "utils/util.h"

#define IIC_STACK_IIC		1
#define IIC_STACK_SCI		2
#define IIC_STACK_COMMS		3

#ifndef IIC_USING_STACK
#define IIC_USING_STACK		IIC_STACK_SCI
#endif

#if IIC_USING_STACK == IIC_STACK_IIC
#define IIC0_MASTER
#define IIC0_MASTER_CB
#elif IIC_USING_STACK == IIC_STACK_SCI
#define IIC0_MASTER			g_sci_i2c1
#define IIC0_MASTER_CB		SCI_I2C1_Callback
#else
#define IIC0_MASTER			g_comms_i2c_device0
#define IIC0_LOWER			g_sci_i2c1
#define IIC0_LOWER_CTRL		UTIL_CONCAT(IIC0_LOWER, _ctrl)
#define IIC0_MASTER_CB		COMMS_I2C_Callback
#endif

#define IIC0_MASTER_CTRL	UTIL_CONCAT(IIC0_MASTER, _ctrl)
#define IIC0_TIMEOUT		2000

#define TAG __FUNCTION__

#ifndef __IIC_DEBUG
#define __IIC_DEBUG 1
#endif

#if __IIC_DEBUG

#include "utils/log.h"

#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#define IIC_LOGD(msg, ...)				LOG_D(TAG, msg, ##__VA_ARGS__)
#define IIC_LOGI(msg, ...)				LOG_I(TAG, msg, ##__VA_ARGS__)
#define IIC_LOGW(msg, ...)				LOG_W(TAG, msg, ##__VA_ARGS__)
#define IIC_LOGE(msg, ...)				LOG_E(TAG, msg, ##__VA_ARGS__)
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#define IIC_LOGD(msg, ...)
#define IIC_LOGI(msg, ...)
#define IIC_LOGW(msg, ...)
#define IIC_LOGE(msg, ...)
#endif

#define IIC_WAIT_OP_DONE(flag)	do { \
									uint32_t __us = IIC0_TIMEOUT * 1000; \
									while (flag.bit.op_done == 0) { \
										UNLIKE_RETURN(flag.bit.error, 0, "Error when wait OP done"); \
										R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS); \
										__us--; \
										if (__us == 0) { \
											LOG_E(__FUNCTION__, "Timeout when wait OP done"); \
											return FSP_ERR_TIMEOUT; \
										} \
									} \
								} while(0)

#define IIC_WAIT_TX_DONE(flag)	do { \
									uint32_t __us = IIC0_TIMEOUT * 1000; \
									while (flag.bit.tx_done == 0) { \
										UNLIKE_RETURN(flag.bit.abort, 0, "Abort when wait TX"); \
										R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS); \
										__us--; \
										if (__us == 0) { \
											LOG_E(__FUNCTION__, "Timeout when wait TX"); \
											return FSP_ERR_TIMEOUT; \
										} \
									} \
								} while(0)
#define IIC_WAIT_RX_DONE(flag)	do { \
									uint32_t __us = IIC0_TIMEOUT * 1000; \
									while (flag.bit.rx_done == 0) { \
										UNLIKE_RETURN(flag.bit.abort, 0, "Abort when wait RX"); \
										R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS); \
										__us--; \
										if (__us == 0) { \
											LOG_E(__FUNCTION__, "Timeout when wait RX"); \
											return FSP_ERR_TIMEOUT; \
										} \
									} \
								} while(0)

union IIC_Flag {
	uint8_t byte;
	struct {
		uint8_t abort : 1;
		uint8_t error : 1;
		uint8_t op_done : 1;
		uint8_t rx_done : 1;
		uint8_t tx_done : 1;
		uint8_t : 3;
	} bit;
};

static volatile union IIC_Flag s_iic0_master_flag;

uint32_t IIC_Init(void)
{
	fsp_err_t err;

#if IIC_USING_STACK == IIC_STACK_COMMS
	err = IIC0_LOWER.p_api->open(IIC0_LOWER.p_ctrl, IIC0_LOWER.p_cfg);
	UNLIKE_RETURN(err, 0, "Lower IIC open failed: %" PRIu32, err);
#endif
	err = IIC0_MASTER.p_api->open(IIC0_MASTER.p_ctrl, IIC0_MASTER.p_cfg);
	IIC_LOGD("IIC Open: %" PRIu32, err);

	return err;
}

uint32_t IIC_ReadMemory(uint32_t slave, uint16_t mem_addr, uint8_t addr_width, uint8_t *rdata, uint16_t rlen)
{
	fsp_err_t err;
	uint8_t wcache[2];
#if IIC_USING_STACK != IIC_STACK_COMMS
	uint8_t wlen;
#else
	rm_comms_write_read_params_t params;
#endif

	if ((addr_width != 8) && (addr_width != 16)) {
	#if __IIC_DEBUG
		LOG_E(__FUNCTION__, "Unsupport mem_addr: %u", mem_addr);
	#endif
		return FSP_ERR_UNSUPPORTED;
	}

#if IIC_USING_STACK != IIC_STACK_COMMS
	if (IIC0_MASTER_CTRL.slave != slave) {
		IIC0_MASTER.p_api->slaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	s_iic0_master_flag.byte = 0;
	if (addr_width == 8) {
		wlen = 1;
		wcache[0] = mem_addr & 0xFF;
	}
	else {
		wlen = 2;
		wcache[0] = (mem_addr >> 8) & 0xFF;
		wcache[1] = mem_addr & 0xFF;
	}
	err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, wlen, true);
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Write failed: 0x%X", err);
	IIC_WAIT_TX_DONE(s_iic0_master_flag);
	s_iic0_master_flag.byte = 0;
	err = IIC0_MASTER.p_api->read(IIC0_MASTER.p_ctrl, rdata, rlen, false);
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Read failed: 0x%X", err);
	IIC_WAIT_RX_DONE(s_iic0_master_flag);
#else
	if (IIC0_LOWER_CTRL.slave != slave) {
		IIC0_LOWER.p_api->slaveAddressSet(IIC0_LOWER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	s_iic0_master_flag.byte = 0;
	if (addr_width == 8) {
		wcache[0] = mem_addr & 0xFF;
		params.src_bytes = 1;
	}
	else {
		wcache[0] = (mem_addr >> 8) & 0xFF;
		wcache[1] = mem_addr & 0xFF;
		params.src_bytes = 2;
	}
	params.p_src = wcache;
	params.p_dest = rdata;
	params.dest_bytes = (uint8_t)rlen;
	err = IIC0_MASTER.p_api->writeRead(IIC0_MASTER.p_ctrl, params);
	UNLIKE_RETURN(err, 0, "Write failed: 0x%X", err);
	IIC_WAIT_OP_DONE(s_iic0_master_flag);
#endif

	return err;
}

uint32_t IIC_ReadReg(uint32_t slave, uint16_t reg_addr, uint8_t addr_width, uint8_t *val, uint8_t val_width)
{
	fsp_err_t err;
	uint8_t wcache[2];
#if IIC_USING_STACK != IIC_STACK_COMMS
	uint8_t rcache[2];
#else
	rm_comms_write_read_params_t params;
#endif

#if IIC_USING_STACK != IIC_STACK_COMMS
	if (IIC0_MASTER_CTRL.slave != slave) {
		IIC0_MASTER.p_api->slaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	s_iic0_master_flag.byte = 0;
	if (addr_width == 8) {
		wcache[0] = reg_addr & 0xFF;
		err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 1, true);
	}
	else {
		wcache[0] = (reg_addr >> 8) & 0xFF;
		wcache[1] = reg_addr & 0xFF;
		err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 2, true);
	}
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Write failed: 0x%X", err);
	IIC_WAIT_TX_DONE(s_iic0_master_flag);
	if (val_width == 8) {
		err = IIC0_MASTER.p_api->read(IIC0_MASTER.p_ctrl, rcache, 1, false);
		IIC_WAIT_RX_DONE(s_iic0_master_flag);
		val[0] = rcache[0];
	}
	else {
		err = IIC0_MASTER.p_api->read(IIC0_MASTER.p_ctrl, rcache, 2, false);
		IIC_WAIT_RX_DONE(s_iic0_master_flag);
		val[0] = rcache[0];
		val[1] = rcache[1];
	}
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Read failed: 0x%X", err);
#else
	if (IIC0_LOWER_CTRL.slave != slave) {
		IIC0_LOWER.p_api->slaveAddressSet(IIC0_LOWER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	s_iic0_master_flag.byte = 0;
	if (addr_width == 8) {
		wcache[0] = reg_addr & 0xFF;
		params.src_bytes = 1;
	}
	else {
		wcache[0] = (reg_addr >> 8) & 0xFF;
		wcache[1] = reg_addr & 0xFF;
		params.src_bytes = 2;
	}
	params.p_src = wcache;
	if (val_width == 8) {
		params.dest_bytes = 1;
	}
	else {
		params.dest_bytes = 2;
	}
	params.p_dest = val;
	err = IIC0_MASTER.p_api->writeRead(IIC0_MASTER.p_ctrl, params);
	UNLIKE_RETURN(err, 0, "Write failed: 0x%X", err);
	IIC_WAIT_OP_DONE(s_iic0_master_flag);
#endif

	return err;
}

uint32_t IIC_Write(uint32_t slave, uint8_t *data, uint8_t length)
{
	fsp_err_t err;

#if IIC_USING_STACK != IIC_STACK_COMMS
	if (IIC0_MASTER_CTRL.slave != slave) {
		IIC0_MASTER.p_api->slaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	s_iic0_master_flag.byte = 0;
	err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, data, length, false);
	UNLIKE_RETURN(err, 0, "Write failed: 0x%X", err);
	IIC_WAIT_TX_DONE(s_iic0_master_flag);
#else
	if (IIC0_LOWER_CTRL.slave != slave) {
		IIC0_LOWER.p_api->slaveAddressSet(IIC0_LOWER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	s_iic0_master_flag.byte = 0;
	err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, data, length);
	UNLIKE_RETURN(err, 0, "Write failed: 0x%X", err);
	IIC_WAIT_OP_DONE(s_iic0_master_flag);
#endif

	return err;
}

uint32_t IIC_WriteMemory(uint32_t slave, uint16_t mem_addr, uint8_t addr_width, const uint8_t *wdata, uint16_t wlen)
{
	fsp_err_t err;
	uint8_t wcache[128];

#if IIC_USING_STACK != IIC_STACK_COMMS
	if (IIC0_MASTER_CTRL.slave != slave) {
		IIC0_MASTER.p_api->slaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
#else
	if (IIC0_LOWER_CTRL.slave != slave) {
		IIC0_LOWER.p_api->slaveAddressSet(IIC0_LOWER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
#endif
	s_iic0_master_flag.byte = 0;
	if (addr_width == 8) {
		wcache[0] = mem_addr & 0xFF;
		if (wlen > 127) {
			memcpy(&wcache[1], wdata, 127);
		#if IIC_USING_STACK != IIC_STACK_COMMS
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 128, false);
		#else
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 128);
		#endif
		}
		else {
			memcpy(&wcache[1], wdata, wlen);
		#if IIC_USING_STACK != IIC_STACK_COMMS
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, wlen + 1, false);
		#else
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, wlen + 1);
		#endif
		}
	}
	else {
		wcache[0] = (mem_addr >> 8) & 0xFF;
		wcache[1] = mem_addr & 0xFF;
		if (wlen > 126) {
			memcpy(&wcache[2], wdata, 126);
		#if IIC_USING_STACK != IIC_STACK_COMMS
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 128, false);
		#else
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 128);
		#endif
		}
		else {
			memcpy(&wcache[2], wdata, wlen);
		#if IIC_USING_STACK != IIC_STACK_COMMS
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, wlen + 2, false);
		#else
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, wlen + 2);
		#endif
		}
	}
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Write failed: 0x%X", err);
#if IIC_USING_STACK != IIC_STACK_COMMS
	IIC_WAIT_TX_DONE(s_iic0_master_flag);
#else
	IIC_WAIT_OP_DONE(s_iic0_master_flag);
#endif

	return err;
}

uint32_t IIC_WriteRead(uint32_t slave, uint8_t *wdata, uint8_t wlen, uint8_t *rdata, uint8_t rlen)
{
	fsp_err_t err;

#if IIC_USING_STACK != IIC_STACK_COMMS
	if (IIC0_MASTER_CTRL.slave != slave) {
		IIC0_MASTER.p_api->slaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	s_iic0_master_flag.byte = 0;
	err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wdata, wlen, true);
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Write failed: 0x%X", err);
	IIC_WAIT_TX_DONE(s_iic0_master_flag);
	s_iic0_master_flag.byte = 0;
	err = IIC0_MASTER.p_api->read(IIC0_MASTER.p_ctrl, rdata, rlen, false);
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Read failed: 0x%X", err);
	IIC_WAIT_RX_DONE(s_iic0_master_flag);
#else
	rm_comms_write_read_params_t params;

	if (IIC0_LOWER_CTRL.slave != slave) {
		IIC0_LOWER.p_api->slaveAddressSet(IIC0_LOWER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	params.src_bytes = wlen;
	params.p_src = wdata;
	params.dest_bytes = rlen;
	params.p_dest = rdata;
	s_iic0_master_flag.byte = 0;
	err = RM_COMMS_I2C_WriteRead(IIC0_MASTER.p_ctrl, params);
	UNLIKE_RETURN(err, 0, "WriteRead failed: 0x%X", err);
	IIC_WAIT_OP_DONE(s_iic0_master_flag);
#endif

	return err;
}

uint32_t IIC_WriteReg(uint32_t slave, uint16_t reg_addr, uint8_t addr_width, uint16_t val, uint8_t val_width)
{
	fsp_err_t err;
	uint8_t wcache[4];

#if IIC_USING_STACK != IIC_STACK_COMMS
	if (IIC0_MASTER_CTRL.slave != slave) {
		IIC0_MASTER.p_api->slaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
#else
	if (IIC0_LOWER_CTRL.slave != slave) {
		IIC0_LOWER.p_api->slaveAddressSet(IIC0_LOWER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
#endif
	s_iic0_master_flag.byte = 0;
	if (addr_width == 8) {
		wcache[0] = reg_addr & 0xFF;
		if (val_width == 8) {
			wcache[1] = val & 0xFF;
		#if IIC_USING_STACK != IIC_STACK_COMMS
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 2, false);
		#else
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 2);
		#endif
		}
		else {
			wcache[1] = (val >> 8) & 0xFF;
			wcache[2] = val & 0xFF;
		#if IIC_USING_STACK != IIC_STACK_COMMS
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 3, false);
		#else
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 3);
		#endif
		}
	}
	else {
		wcache[0] = (reg_addr >> 8) & 0xFF;
		wcache[1] = reg_addr & 0xFF;
		if (val_width == 8) {
			wcache[2] = val & 0xFF;
		#if IIC_USING_STACK != IIC_STACK_COMMS
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 3, false);
		#else
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 3);
		#endif
		}
		else {
			wcache[2] = (val >> 8) & 0xFF;
			wcache[3] = val & 0xFF;
		#if IIC_USING_STACK != IIC_STACK_COMMS
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 4, false);
		#else
			err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 4);
		#endif
		}
	}
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Write failed: 0x%X", err);
#if IIC_USING_STACK != IIC_STACK_COMMS
	IIC_WAIT_TX_DONE(s_iic0_master_flag);
#else
	IIC_WAIT_OP_DONE(s_iic0_master_flag);
#endif

	return err;
}

uint32_t IIC_SCCB_ReadReg(uint32_t slave, uint16_t reg, uint8_t reg_width, uint8_t *val)
{
	fsp_err_t err;
	uint8_t wcache[2];
#if IIC_USING_STACK == IIC_STACK_COMMS
	rm_comms_write_read_params_t params;
#endif

	if ((reg_width != 8) && (reg_width != 16)) {
	#if __IIC_DEBUG
		LOG_E(__FUNCTION__, "Unsupport reg_width: %u", reg_width);
	#endif
		return FSP_ERR_UNSUPPORTED;
	}

#if IIC_USING_STACK != IIC_STACK_COMMS
	if (IIC0_MASTER_CTRL.slave != slave) {
		IIC0_MASTER.p_api->slaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	s_iic0_master_flag.byte = 0;
	if (reg_width == 8) {
		wcache[0] = reg & 0xFF;
		err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 1, true);
	}
	else {
		wcache[0] = (uint8_t)((reg >> 8) & 0xFF);
		wcache[1] = (uint8_t)(reg & 0xFF);
		err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 2, true);
	}
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Write failed: 0x%X", err);
	IIC_WAIT_TX_DONE(s_iic0_master_flag);
	s_iic0_master_flag.byte = 0;
	err = IIC0_MASTER.p_api->read(IIC0_MASTER.p_ctrl, val, 1, false);
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Read failed: 0x%X", err);
	IIC_WAIT_RX_DONE(s_iic0_master_flag);
#else
	if (IIC0_LOWER_CTRL.slave != slave) {
		IIC0_LOWER.p_api->slaveAddressSet(IIC0_LOWER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
	s_iic0_master_flag.byte = 0;
	if  (reg_width == 8) {
		wcache[0] = reg & 0xFF;
		params.src_bytes = 1;
	}
	else {
		wcache[0] = (reg >> 8) & 0xFF;
		wcache[1] = reg & 0xFF;
		params.src_bytes = 2;
	}
	params.dest_bytes = 1;
	params.p_dest = val;
	err = IIC0_MASTER.p_api->writeRead(IIC0_MASTER.p_ctrl, params);
	UNLIKE_RETURN(err, 0, "WriteRead failed: 0x%X", err);
	IIC_WAIT_OP_DONE(s_iic0_master_flag);
#endif

	return err;
}

uint32_t IIC_SCCB_WriteReg(uint32_t slave, uint16_t reg, uint8_t reg_width, uint8_t val)
{
	fsp_err_t err;
	uint8_t wcache[3];

	if ((reg_width != 8) && (reg_width != 16)) {
	#if __IIC_DEBUG
		LOG_E(__FUNCTION__, "Unsupport reg_width: %u", reg_width);
	#endif
		return FSP_ERR_UNSUPPORTED;
	}

#if IIC_USING_STACK != IIC_STACK_COMMS
	if (IIC0_MASTER_CTRL.slave != slave) {
		IIC0_MASTER.p_api->slaveAddressSet(IIC0_MASTER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
#else
	if (IIC0_LOWER_CTRL.slave != slave) {
		IIC0_LOWER.p_api->slaveAddressSet(IIC0_LOWER.p_ctrl, slave, I2C_MASTER_ADDR_MODE_7BIT);
	}
#endif
	s_iic0_master_flag.byte = 0;
	if (reg_width == 8) {
		wcache[0] = reg & 0xFF;
		wcache[1] = val;
	#if IIC_USING_STACK != IIC_STACK_COMMS
		err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 2, false);
	#else
		err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 2);
	#endif
	}
	else {
		wcache[0] = (uint8_t)((reg >> 8) & 0xFF);
		wcache[1] = (uint8_t)(reg & 0xFF);
		wcache[2] = val;
	#if IIC_USING_STACK != IIC_STACK_COMMS
		err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 3, false);
	#else
		err = IIC0_MASTER.p_api->write(IIC0_MASTER.p_ctrl, wcache, 3);
	#endif
	}
	UNLIKE_RETURN(err, 0, "R_IIC_MASTER_Write failed: 0x%X", err);
#if IIC_USING_STACK != IIC_STACK_COMMS
	IIC_WAIT_TX_DONE(s_iic0_master_flag);
#else
	IIC_WAIT_OP_DONE(s_iic0_master_flag);
#endif

	return err;
}

#if IIC_USING_STACK != IIC_STACK_COMMS
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
#else
void IIC0_MASTER_CB(rm_comms_callback_args_t *p_args)
{
	switch (p_args->event) {
	case RM_COMMS_EVENT_OPERATION_COMPLETE:
		s_iic0_master_flag.bit.op_done = 1;
		break;
	case RM_COMMS_EVENT_TX_OPERATION_COMPLETE:
		s_iic0_master_flag.bit.tx_done = 1;
		break;
	case RM_COMMS_EVENT_RX_OPERATION_COMPLETE:
		s_iic0_master_flag.bit.rx_done = 1;
		break;
	case RM_COMMS_EVENT_ERROR:
		s_iic0_master_flag.bit.error = 1;
		break;
	default:
		break;
	}
}
#endif
