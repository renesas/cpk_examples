#include <inttypes.h>
#include "i2c_comms.h"

/* 注意事项：
 * 	- 如果多个 rm_comms_i2c stack 对应多个不同的 r_iic_master 或 r_sci_b_i2c，那么最好使用多个不同的回调
 * 	  这样能避免多个 iic 外设等待同一个标志导致资源浪费。 */
#define I2C_COMMS_CB0			COMMS_I2C_Callback
#define I2C_COMMS_TIMEOUT		200
#define I2C_COMMS_WCACHE_SIZE	128

#ifndef __i2C_COMMS_DEBUG
#define __i2C_COMMS_DEBUG	1
#endif

#if __i2C_COMMS_DEBUG
#include "utils/log.h"
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#define I2C_LOGD(msg, ...)				LOG_D(TAG, msg, ##__VA_ARGS__)
#define I2C_LOGI(msg, ...)				LOG_I(TAG, msg, ##__VA_ARGS__)
#define I2C_LOGW(msg, ...)				LOG_W(TAG, msg, ##__VA_ARGS__)
#define I2C_LOGE(msg, ...)				LOG_E(TAG, msg, ##__VA_ARGS__)
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#define I2C_LOGD(msg, ...)
#define I2C_LOGI(msg, ...)
#define I2C_LOGW(msg, ...)
#define I2C_LOGE(msg, ...)
#endif

#define IIC_WAIT_OP_DONE(flag)	do { \
									uint32_t __us = I2C_COMMS_TIMEOUT * 1000; \
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

union I2C_COMMS_Flag {
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

static uint8_t s_wcache[I2C_COMMS_WCACHE_SIZE];
static union I2C_COMMS_Flag s_i2c_comms_flags[1];

uint32_t I2C_COMMS_Init(const rm_comms_instance_t *i2c)
{
	rm_comms_i2c_bus_extended_cfg_t *p_extend = (rm_comms_i2c_bus_extended_cfg_t *)i2c->p_cfg->p_extend;
	const i2c_master_instance_t *p_lower_inst = (const i2c_master_instance_t *)p_extend->p_driver_instance;

	p_lower_inst->p_api->open(p_lower_inst->p_ctrl, p_lower_inst->p_cfg);
	i2c->p_api->open(i2c->p_ctrl, i2c->p_cfg);

	return FSP_SUCCESS;
}

uint32_t I2C_COMMS_ReadMem(const rm_comms_instance_t *i2c, uint16_t mem_addr, uint8_t addr_width, uint8_t *rdata, uint16_t rlen)
{
	uint8_t wcache[2];
	uint32_t err;
	rm_comms_write_read_params_t params;

	s_i2c_comms_flags[0].byte = 0;
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
	err = i2c->p_api->writeRead(i2c->p_ctrl, params);
	UNLIKE_RETURN(err, 0, "Write failed: 0x%" PRIX32, err);
	IIC_WAIT_OP_DONE(s_i2c_comms_flags[0]);

	return err;
}

uint32_t I2C_COMMS_ReadReg(const rm_comms_instance_t *i2c, uint16_t reg_addr, uint8_t addr_width, uint8_t *val, uint8_t val_width)
{
	uint8_t wcache[2];
	uint32_t err;
	rm_comms_write_read_params_t params;

	s_i2c_comms_flags[0].byte = 0;
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
	err = i2c->p_api->writeRead(i2c->p_ctrl, params);
	UNLIKE_RETURN(err, 0, "Write failed: 0x%" PRIX32, err);
	IIC_WAIT_OP_DONE(s_i2c_comms_flags[0]);

	return err;
}

uint32_t I2C_COMMS_Write(const rm_comms_instance_t *i2c, uint8_t *data, uint16_t length)
{
	uint16_t i;
	uint32_t err;

	uint8_t *pw = data;
	uint16_t repeat = length / UINT8_MAX;
	uint16_t remain = length % UINT8_MAX;

	for (i = 0; i < repeat; i++) {
		s_i2c_comms_flags[0].byte = 0;
		err = i2c->p_api->write(i2c->p_ctrl, pw, UINT8_MAX);
		UNLIKE_RETURN(err, 0, "Write failed: 0x%" PRIX32, err);
		IIC_WAIT_OP_DONE(s_i2c_comms_flags[0]);
		pw = &pw[UINT8_MAX];
	}
	if (remain) {
		s_i2c_comms_flags[0].byte = 0;
		err = i2c->p_api->write(i2c->p_ctrl, pw, (uint8_t)remain);
		UNLIKE_RETURN(err, 0, "Write failed: 0x%" PRIX32, err);
		IIC_WAIT_OP_DONE(s_i2c_comms_flags[0]);
	}

	return err;
}

uint32_t I2C_COMMS_WriteMem(const rm_comms_instance_t *i2c, uint16_t mem_addr, uint8_t addr_width, const uint8_t *wdata, uint16_t wlen)
{
	uint32_t err;

	s_i2c_comms_flags[0].byte = 0;
	if (addr_width == 8) {
		s_wcache[0] = mem_addr & 0xFF;
		if (wlen > (I2C_COMMS_WCACHE_SIZE - 1)) {
			memcpy(&s_wcache[1], wdata, I2C_COMMS_WCACHE_SIZE - 1);
			err = i2c->p_api->write(i2c->p_ctrl, s_wcache, (uint8_t)I2C_COMMS_WCACHE_SIZE);
		}
		else {
			memcpy(&s_wcache[1], wdata, wlen);
			err = i2c->p_api->write(i2c->p_ctrl, s_wcache, wlen + 1);
		}
	}
	else {
		s_wcache[0] = (mem_addr >> 8) & 0xFF;
		s_wcache[1] = mem_addr & 0xFF;
		if (wlen > (I2C_COMMS_WCACHE_SIZE - 2)) {
			memcpy(&s_wcache[1], wdata, I2C_COMMS_WCACHE_SIZE - 2);
			err = i2c->p_api->write(i2c->p_ctrl, s_wcache, (uint8_t)I2C_COMMS_WCACHE_SIZE);
		}
		else {
			memcpy(&s_wcache[2], wdata, wlen);
			err = i2c->p_api->write(i2c->p_ctrl, s_wcache, wlen + 2);
		}
	}
	UNLIKE_RETURN(err, 0, "Write failed: 0x%" PRIX32, err);
	IIC_WAIT_OP_DONE(s_i2c_comms_flags[0]);

	return err;
}

uint32_t I2C_COMMS_WriteRead(const rm_comms_instance_t *i2c, uint8_t *wdata, uint16_t wlen, uint8_t *rdata, uint16_t rlen)
{
	uint32_t err;
	rm_comms_write_read_params_t params;

	params.src_bytes = (uint8_t)wlen;
	params.p_src = wdata;
	params.dest_bytes = (uint8_t)rlen;
	params.p_dest = rdata;
	s_i2c_comms_flags[0].byte = 0;
	err = i2c->p_api->writeRead(i2c->p_ctrl, params);
	UNLIKE_RETURN(err, 0, "Write failed: 0x%" PRIX32, err);
	IIC_WAIT_OP_DONE(s_i2c_comms_flags[0]);

	return err;
}

uint32_t I2C_COMMS_WriteReg(const rm_comms_instance_t *i2c, uint16_t reg_addr, uint8_t addr_width, uint16_t val, uint8_t val_width)
{
	uint32_t err;

	s_i2c_comms_flags[0].byte = 0;
	if (addr_width == 8) {
		s_wcache[0] = reg_addr & 0xFF;
		if (val_width == 8) {
			s_wcache[1] = val & 0xFF;
			err = i2c->p_api->write(i2c->p_ctrl, s_wcache, 2);
		}
		else {
			s_wcache[1] = (val >> 8) & 0xFF;
			s_wcache[2] = val & 0xFF;
			err = i2c->p_api->write(i2c->p_ctrl, s_wcache, 3);
		}
	}
	else {
		s_wcache[0] = (reg_addr >> 8) & 0xFF;
		s_wcache[1] = reg_addr & 0xFF;
		if (val_width == 8) {
			s_wcache[2] = val & 0xFF;
			err = i2c->p_api->write(i2c->p_ctrl, s_wcache, 3);
		}
		else {
			s_wcache[2] = (val >> 8) & 0xFF;
			s_wcache[3] = val & 0xFF;
			err = i2c->p_api->write(i2c->p_ctrl, s_wcache, 4);
		}
	}
	UNLIKE_RETURN(err, 0, "Write failed: 0x%" PRIX32, err);
	IIC_WAIT_OP_DONE(s_i2c_comms_flags[0]);

	return err;
}

uint32_t I2C_COMMS_SCCB_ReadReg(const rm_comms_instance_t *i2c, uint16_t reg, uint8_t reg_width, uint8_t *val)
{
	uint32_t err;
	rm_comms_write_read_params_t params;

	s_i2c_comms_flags[0].byte = 0;
	if (reg_width == 8) {
		s_wcache[0] = reg & 0xFF;
		params.src_bytes = 1;
	}
	else {
		s_wcache[1] = (reg >> 8) & 0xFF;
		s_wcache[2] = reg & 0xFF;
		params.src_bytes = 2;
	}
	params.dest_bytes = 1;
	params.p_dest = val;
	err = i2c->p_api->writeRead(i2c->p_ctrl, params);
	UNLIKE_RETURN(err, 0, "Write failed: 0x%" PRIX32, err);
	IIC_WAIT_OP_DONE(s_i2c_comms_flags[0]);

	return err;
}

uint32_t I2C_COMMS_SCCB_WriteReg(const rm_comms_instance_t *i2c, uint16_t reg, uint8_t reg_width, uint8_t val)
{
	uint32_t err;

	s_i2c_comms_flags[0].byte = 0;
	if (reg_width == 8) {
		s_wcache[0] = reg & 0xFF;
		s_wcache[1] = val;
		err = i2c->p_api->write(i2c->p_ctrl, s_wcache, 2);
	}
	else {
		s_wcache[0] = (reg >> 8) & 0xFF;
		s_wcache[1] = reg & 0xFF;
		s_wcache[2] = val;
		err = i2c->p_api->write(i2c->p_ctrl, s_wcache, 3);
	}
	UNLIKE_RETURN(err, 0, "Write failed: 0x%" PRIX32, err);
	IIC_WAIT_OP_DONE(s_i2c_comms_flags[0]);

	return err;
}

void I2C_COMMS_CB0(rm_comms_callback_args_t *p_args)
{
	switch (p_args->event) {
	case RM_COMMS_EVENT_OPERATION_COMPLETE:
		s_i2c_comms_flags[0].bit.op_done = 1;
		break;
	case RM_COMMS_EVENT_TX_OPERATION_COMPLETE:
		s_i2c_comms_flags[0].bit.tx_done = 1;
		break;
	case RM_COMMS_EVENT_RX_OPERATION_COMPLETE:
		s_i2c_comms_flags[0].bit.rx_done = 1;
		break;
	case RM_COMMS_EVENT_ERROR:
		s_i2c_comms_flags[0].bit.error = 1;
		break;
	default:
		break;
	}
}
