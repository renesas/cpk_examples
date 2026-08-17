#include "hal_data.h"
#include "i3c.h"

#define I3C_INSTANCE	g_i3c
#define I3C_CALLBACK	I3C_Callback
#define I3C_RCACHE_SIZE	32

#define TAG __FUNCTION__

#ifndef __I3C_DEBUG
#define __I3C_DEBUG	1
#endif

#if __I3C_DEBUG
#include "utils/log.h"
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(TAG, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(TAG, msg, ##__VA_ARGS__); return v; }
#define I3C_LOGD(msg, ...)				LOG_D(TAG, msg, ##__VA_ARGS__)
#define I3C_LOGW(msg, ...)				LOG_W(TAG, msg, ##__VA_ARGS__)
#define I3C_LOGE(msg, ...)				LOG_E(TAG, msg, ##__VA_ARGS__)
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#define I3C_LOGD(msg, ...)
#define I3C_LOGW(msg, ...)
#define I3C_LOGE(msg, ...)
#endif

#define I3C_CLEAR_EVENT(v, e)			v &= (uint32_t)(~(0x01 << e))
#define I3C_SET_EVENT(v, e)				v |= (0x01 << e)
#define I3C_WAIT_EVENT(v, e, t, m, ...)	do { \
											uint32_t __timeout_us = t * 1000; \
											while ((v & (0x01 << e)) == 0) { \
												if (__timeout_us == 0) { \
													if (m != NULL) { I3C_LOGD(m, ##__VA_ARGS__); } \
													return FSP_ERR_TIMEOUT; \
												} \
												R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS); \
											} \
										} while (0)

static uint8_t s_ibi_cache[I3C_RCACHE_SIZE];
static volatile uint32_t s_i3c_event;
static volatile uint32_t s_i3c_ibi_event;
static volatile uint32_t s_i3c_trans_stat;
static i3c_device_cfg_t s_device_cfg;
static i3c_device_table_cfg_t s_device_table_cfg;

uint32_t I3C_CheckHotjoin(void)
{
	uint32_t err;
	i3c_command_descriptor_t cmd;

	I3C_CLEAR_EVENT(s_i3c_ibi_event, I3C_IBI_TYPE_HOT_JOIN);
	err = R_I3C_IbiRead(I3C_INSTANCE.p_ctrl, s_ibi_cache, I3C_RCACHE_SIZE);
	UNLIKE_RETURN(err, 0, "IbiRead failed: %u", err);

	I3C_CLEAR_EVENT(s_i3c_event, I3C_EVENT_ADDRESS_ASSIGNMENT_COMPLETE);
	err = R_I3C_DynamicAddressAssignmentStart(I3C_INSTANCE.p_ctrl, I3C_ADDRESS_ASSIGNMENT_MODE_ENTDAA, 0, 1);
	UNLIKE_RETURN(err, 0, "DynamicAddressAssignment failed: %u", err);
	I3C_WAIT_EVENT(s_i3c_event, I3C_EVENT_ADDRESS_ASSIGNMENT_COMPLETE, 2000, "Wait event: ADDRESS_ASSIGNMENT_COMPLETE timeout");

	cmd.command_code = I3C_CCC_BROADCAST_RSTDAA;
	cmd.length = 0;
	cmd.p_buffer = NULL;
	cmd.restart = false;
	cmd.rnw = false;
	I3C_CLEAR_EVENT(s_i3c_event, I3C_EVENT_COMMAND_COMPLETE);
	err = R_I3C_CommandSend(I3C_INSTANCE.p_ctrl, &cmd);
	UNLIKE_RETURN(err, 0, "Send RSTDAA failed: %u", err);
	I3C_WAIT_EVENT(s_i3c_event, I3C_EVENT_COMMAND_COMPLETE, 2000, "Wait event: COMMAND_COMPLETE timeout");

	I3C_WAIT_EVENT(s_i3c_event, I3C_EVENT_IBI_READ_COMPLETE, 2000, "Wait event: IBI_READ_COMPLETE timeout");
	I3C_WAIT_EVENT(s_i3c_ibi_event, I3C_IBI_TYPE_HOT_JOIN, 2000, "Wait IBI event: HOT_JOIN timeout");

	I3C_CLEAR_EVENT(s_i3c_event, I3C_EVENT_ADDRESS_ASSIGNMENT_COMPLETE);
	err = R_I3C_DynamicAddressAssignmentStart(I3C_INSTANCE.p_ctrl, I3C_ADDRESS_ASSIGNMENT_MODE_ENTDAA, 0, 1);
	UNLIKE_RETURN(err, 0, "DynamicAddressAssignment failed: %u", err);
	I3C_WAIT_EVENT(s_i3c_event, I3C_EVENT_ADDRESS_ASSIGNMENT_COMPLETE, 2000, "Wait event: ADDRESS_ASSIGNMENT_COMPLETE timeout");
	I3C_CLEAR_EVENT(s_i3c_ibi_event, I3C_IBI_TYPE_HOT_JOIN);

	return 0;
}

uint32_t I3C_Init(void)
{
	uint32_t err;
	ioport_instance_ctrl_t pin_ctrl;
	ioport_pin_cfg_t pin_cfg_data[3];
    ioport_cfg_t pin_cfg;

	pin_cfg_data[0].pin = BSP_IO_PORT_04_PIN_00;
	pin_cfg_data[0].pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_CFG_PULLUP_ENABLE | IOPORT_PERIPHERAL_IIC;
	pin_cfg_data[1].pin = BSP_IO_PORT_04_PIN_01;
	pin_cfg_data[1].pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_CFG_PULLUP_ENABLE | IOPORT_PERIPHERAL_IIC;
	pin_cfg_data[2].pin = BSP_IO_PORT_04_PIN_13;
	pin_cfg_data[2].pin_cfg = IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_HIGH;
	pin_cfg.number_of_pins = 3;
	pin_cfg.p_pin_cfg_data = (ioport_pin_cfg_t const *)&pin_cfg_data;
    pin_cfg.p_extend = NULL;
    R_IOPORT_Open(&pin_ctrl, &pin_cfg);

	err = R_I3C_Open(I3C_INSTANCE.p_ctrl, I3C_INSTANCE.p_cfg);
	UNLIKE_RETURN(err, 0, "Open failed: %u", err);

	s_device_cfg.dynamic_address = 0x70;
	s_device_cfg.static_address = 0x70;
	err = R_I3C_DeviceCfgSet(I3C_INSTANCE.p_ctrl, &s_device_cfg);
	UNLIKE_RETURN(err, 0, "DeviceCfgSet failed: %u", err);

	s_device_table_cfg.device_protocol = I3C_DEVICE_PROTOCOL_I3C;
	s_device_table_cfg.dynamic_address = 0x71;
	s_device_table_cfg.ibi_accept = true;
	s_device_table_cfg.ibi_payload = true;
	s_device_table_cfg.master_request_accept = false;
	err = R_I3C_MasterDeviceTableSet(I3C_INSTANCE.p_ctrl, 0x00, &s_device_table_cfg);
	UNLIKE_RETURN(err, 0, "MasterDeviceTableSet failed: %u", err);

	err = R_I3C_Enable(I3C_INSTANCE.p_ctrl);
	UNLIKE_RETURN(err, 0, "Enable failed: %u", err);

	err = I3C_CheckHotjoin();
	UNLIKE_RETURN(err, 0, "CheckHotjoin failed: %u", err);

#if __I3C_DEBUG
	LOG_D(TAG, "bcr: 0x%X", s_device_cfg.slave_info.bcr);
	LOG_D(TAG, "dcr: 0x%X", s_device_cfg.slave_info.dcr);
	uint64_t pid = 0;
	memcpy(&pid, s_device_cfg.slave_info.pid, 6);
	LOG_D(TAG, "pid: 0x%llX", pid);
#endif

	return 0;
}

uint32_t I3C_ReadMemory(uint8_t reg, uint8_t *data, uint32_t length)
{
	uint32_t err;

	if ((data == NULL) || (length == 0)) {
		return FSP_ERR_INVALID_ARGUMENT;
	}

	err = R_I3C_DeviceSelect(I3C_INSTANCE.p_ctrl, 0, I3C_BITRATE_MODE_I3C_SDR0_STDBR);
	UNLIKE_RETURN(err, 0, "DeviceSelect failed: %u", err);
	I3C_CLEAR_EVENT(s_i3c_event, I3C_EVENT_WRITE_COMPLETE);
	err = R_I3C_Write(I3C_INSTANCE.p_ctrl, &reg, 1, true);
	UNLIKE_RETURN(err, 0, "Write register address failed: %u", err);
	I3C_WAIT_EVENT(s_i3c_event, I3C_EVENT_WRITE_COMPLETE, 3000, "Wait event: WRITE_COMPLETE timeout");

	I3C_CLEAR_EVENT(s_i3c_event, I3C_EVENT_READ_COMPLETE);
	err = R_I3C_Read(I3C_INSTANCE.p_ctrl, data, length, false);
	UNLIKE_RETURN(err, 0, "Read register data failed: %u", err);
	I3C_WAIT_EVENT(s_i3c_event, I3C_EVENT_READ_COMPLETE, 3000, "Wait event: READ_COMPLETE timeout");

	return 0;
}

uint32_t I3C_WriteMemory(uint8_t reg, const uint8_t *data, uint32_t length)
{
	fsp_err_t err;
	uint8_t wcache[128];

	if ((data == NULL) || (length == 0)) {
		return FSP_ERR_INVALID_ARGUMENT;
	}
	if (length > 127) {
		return FSP_ERR_OUT_OF_MEMORY;
	}

	wcache[0] = reg;
	memcpy(&wcache[1], data, length);
	I3C_CLEAR_EVENT(s_i3c_event, I3C_EVENT_WRITE_COMPLETE);
	err = R_I3C_Write(I3C_INSTANCE.p_ctrl, wcache, length + 1, false);
	UNLIKE_RETURN(err, 0, "Write failed: %u", err);
	I3C_WAIT_EVENT(s_i3c_event, I3C_EVENT_WRITE_COMPLETE, 3000, "Wait event: WRITE_COMPLETE timeout");

	return 0;
}

void I3C_CALLBACK(i3c_callback_args_t const *const p_args)
{
	s_i3c_event |= (0x01 << p_args->event);

	switch (p_args->event) {
	case I3C_EVENT_ENTDAA_ADDRESS_PHASE:
		s_device_cfg.slave_info.bcr = p_args->p_slave_info->bcr;
		s_device_cfg.slave_info.dcr = p_args->p_slave_info->dcr;
		memcpy(s_device_cfg.slave_info.pid, p_args->p_slave_info->pid, 6);
		break;
	case I3C_EVENT_IBI_READ_COMPLETE:
		s_i3c_ibi_event |= (0x01 << p_args->ibi_type);
		switch (p_args->ibi_type) {
		case I3C_IBI_TYPE_INTERRUPT:
			I3C_LOGD("IBI Type: INTERRUPT");
			break;
		case I3C_IBI_TYPE_HOT_JOIN:
			I3C_LOGD("IBI Type: HOT_JOIN");
			break;
		case I3C_IBI_TYPE_MASTERSHIP_REQUEST:
			break;
		default:
			break;
		}
		break;
	case I3C_EVENT_IBI_READ_BUFFER_FULL:
		break;
	case I3C_EVENT_READ_BUFFER_FULL:
		break;
	case I3C_EVENT_IBI_WRITE_COMPLETE:
		break;
	case I3C_EVENT_HDR_EXIT_PATTERN_DETECTED:
		break;
	case I3C_EVENT_ADDRESS_ASSIGNMENT_COMPLETE:
		break;
	case I3C_EVENT_COMMAND_COMPLETE:
		break;
	case I3C_EVENT_WRITE_COMPLETE:
		s_i3c_trans_stat = p_args->event_status;
		if (p_args->event_status == I3C_EVENT_STATUS_NACK) {
			I3C_LOGW("NACK when write");
		}
		break;
	case I3C_EVENT_READ_COMPLETE:
		s_i3c_trans_stat = p_args->event_status;
		if (p_args->event_status == I3C_EVENT_STATUS_NACK) {
			I3C_LOGW("NACK when read");
		}
		break;
	case I3C_EVENT_TIMEOUT_DETECTED:
		break;
	case I3C_EVENT_INTERNAL_ERROR:
		break;
	case I3C_EVENT_SDA_WRITE_COMPLETE:
		break;
	default:
		break;
	}
}
