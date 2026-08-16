#include "fusb303b.h"

#define REG_DEVICE_ID		0x01
#define REG_DEVICE_TYPE		0X03
#define REG_PORTROLE		0x03
#define REG_CONTROL			0x04
#define REG_CONTROL_1		0x05
#define REG_MANUAL			0x09
#define REG_RESET			0x0A
#define REG_MASK			0x0E
#define REG_MASK_1			0x0F
#define REG_STATUS			0x11
#define REG_STATUS_1		0x12
#define REG_TYPE			0x13
#define REG_INTERRUPT		0x14
#define REG_INTERRUPT_1		0x15

#define REG_CONTROL_INT_MASK		0x01
#define REG_CONTROL_1_ENABLE		0x08

#define REG_STATUS_ATTACH			0x01
#define REG_STATUS_BC_LVL			0x06
#define REG_STATUS_VBUSOK			0x08
#define REG_STATUS_ORIENT			0x30
#define REG_STATUS_VSAFE0V			0x40
#define REG_STATUS_AUTOSNK			0x80

#define REG_STATUS_1_REMEDY			0x01
#define REG_STATUS_1_FAULT			0x02
#define REG_STATUS_1_RESERVED		0xFC

#define REG_TYPE_AUDIO				0x01
#define REG_TYPE_AUDIOVBUS			0x02
#define REG_TYPE_ACTIVECABLE		0x04
#define REG_TYPE_SOURCE				0x08
#define REG_TYPE_SINK				0x10
#define REG_TYPE_DEBUGSNK			0x20
#define REG_TYPE_DEBUGSRC			0x40
#define REG_TYPE_RESERVED			0x80

#ifndef __FUSB303B_DEBUG
#define __FUSB303B_DEBUG 0
#endif

#if __FUSB303B_DEBUG
#include "utils/log.h"
#endif

static uint8_t s_powerd_by_jusb;

uint32_t FUSB303B_Check(FUSB303_Status *status)
{
	uint8_t data_read[5];
	uint8_t data_write[4];
	volatile uint8_t t;

	FUSB303B_Read(REG_STATUS, data_read, 5);
#if __FUSB303B_DEBUG
	LOG_D(__FUNCTION__, "Status: 0x%02X", data_read[0]);
	LOG_D(__FUNCTION__, "Status_1: 0x%02X", data_read[1]);
	LOG_D(__FUNCTION__, "Type: 0x%02X", data_read[2]);
	LOG_D(__FUNCTION__, "Interrupt: 0x%02X", data_read[3]);
	LOG_D(__FUNCTION__, "Interrupt_1: 0x%02X", data_read[4]);
#endif

	if (data_read[3] != 0) {
		data_write[0] = REG_INTERRUPT;
		data_write[1] = 0x7F;
		FUSB303B_Write(data_write, 2);
	}
	if (data_read[4] != 0) {
		data_write[0] = REG_INTERRUPT_1;
		data_write[1] = 0x7F;
		FUSB303B_Write(data_write, 2);
	}

	if ((data_read[3] & 0x02) || ((data_read[0] & REG_STATUS_ATTACH) == 0)) {
		status->attached = 0;
		status->powerd_by_jusb = 0;
		status->type = DETECT_TYPE_UNKNOW;
		status->cc_orient = DETECT_CC_UNCONNECTION;
		status->current_level = CURRENT_LEVEL_UNCONNECTION;
		s_powerd_by_jusb = 0;

		if (data_read[0] & REG_STATUS_VBUSOK) {
			status->vbus_connect = 1;
		}
		else {
			status->vbus_connect = 0;
		}

		return 0;
	}

	status->attached = 1;
	status->powerd_by_jusb = s_powerd_by_jusb;
	switch (data_read[2]) {
	case 0x01:
		status->type = DETECT_TYPE_AUDIO;
		break;
	case 0x02:
		status->type = DETECT_TYPE_AUDIOVBUS;
		break;
	case 0x04:
		status->type = DETECT_TYPE_ACTIVECABLE;
		break;
	case 0x08:
		status->type = DETECT_TYPE_SOURCE;
		break;
	case 0x10:
		status->type = DETECT_TYPE_SINK;
		break;
	case 0x20:
		status->type = DETECT_TYPE_DEBUGSNK;
		break;
	case 0x40:
		status->type = DETECT_TYPE_DEBUGSRC;
		break;
	default:
		status->type = DETECT_TYPE_UNKNOW;
		break;
	}

	t = (data_read[0] & REG_STATUS_ORIENT) >> 4;
	switch (t) {
	case 0x00:
		status->cc_orient = DETECT_CC_UNCONNECTION;
		break;
	case 0x01:
		status->cc_orient = DETECT_CC_IN_CC1_A5;
		break;
	case 0x02:
		status->cc_orient = DETECT_CC_IN_CC2_B5;
		break;
	default:
		status->cc_orient = DETECT_CC_FAULT;
		break;
	}

	t = (data_read[0] & REG_STATUS_BC_LVL) >> 1;
	switch (t) {
	case 0x01:
		status->current_level = CURRENT_LEVEL_DEFAULT;
		break;
	case 0x02:
		status->current_level = CURRENT_LEVEL_1_DOT_5_A;
		break;
	case 0x03:
		status->current_level = CURRENT_LEVEL_3_DOT_0_A;
		break;
	default:
		status->current_level = CURRENT_LEVEL_UNCONNECTION;
		break;
	}

	if (data_read[0] & REG_STATUS_VBUSOK) {
		status->vbus_connect = 1;
	}
	else {
		status->vbus_connect = 0;
	}

	return 0;
}

uint32_t FUSB303B_Init(void)
{
	FUSB303_Status status;
	uint8_t data_read[2];
	uint8_t data_write[4];

	FUSB303B_Read(REG_DEVICE_ID, data_write, 2);
	if (data_write[0] != 0x10) {
	#if __FUSB303B_DEBUG
		LOG_E(__FUNCTION__, "Invalid ID[0x%02X] != 0x10", data_write[0]);
	#endif
		return 1;
	}
	if (data_write[1] != 0x03) {
	#if __FUSB303B_DEBUG
		LOG_E(__FUNCTION__, "Invalid Type[0x%02X] != 0x03", data_write[1]);
	#endif
		return 1;
	}

	/* Reset */
	FUSB303B_ENPinControl(1);
	FUSB303B_Delay(10);
	FUSB303B_ENPinControl(0);
	FUSB303B_Delay(100);

	/* Enable IIC interface */
	FUSB303B_Read(REG_CONTROL_1, data_read, 1);
	if ((data_read[0] & REG_CONTROL_1_ENABLE) == 0x00) {
		data_write[0] = REG_CONTROL_1;
		data_write[1] = data_read[0] | REG_CONTROL_1_ENABLE;
		FUSB303B_Write(data_write, 2);
		FUSB303B_Delay(10);
		FUSB303B_Read(REG_CONTROL_1, data_read, 1);
		if ((data_read[0] & REG_CONTROL_1_ENABLE) == 0x00) {
		#if __FUSB303B_DEBUG
			LOG_W(__FUNCTION__, "Set ENABLE failed [0x%02X]", data_read[0]);
		#endif
		}
	}

	/** Set PORTROLE 
	 * BIT[7] = 0: Reserved
	 * BIT[6] = 1: When a debug accessory is found, will continue to orientation detection if CC is on CC1 or CC2
	 * BIT[5:4] = 00: Normal DRP detection
	 * BIT[3] = 1: Enable Audio Accessory Support
	 * BIT[2] = 1: Device as a Dual Role Port
	 * BIT[1] = 0: Not as a sink
	 * BIT[0] = 0: Not as a source */
	data_write[0] = REG_PORTROLE;
	data_write[1] = 0x4C;
	FUSB303B_Write(data_write, 2);
	FUSB303B_Read(REG_PORTROLE, data_read, 1);
	if (data_read[0] != 0x4C) {
	#if __FUSB303B_DEBUG
		LOG_W(__FUNCTION__, "Set PORTROLE failed [0x%02X]", data_read[0]);
	#endif
	}

	/* Disable golbal interrupt, check wheather in SINK immediately */
	FUSB303B_Read(REG_CONTROL, data_read, 1);
	if ((data_read[0] & REG_CONTROL_INT_MASK) == 0) {
		data_write[0] = REG_CONTROL;
		data_write[1] = data_read[0] | REG_CONTROL_INT_MASK;
		FUSB303B_Write(data_write, 2);
		FUSB303B_Read(REG_CONTROL, data_read, 1);
		if ((data_read[0] & REG_CONTROL_INT_MASK) == 0) {
		#if __FUSB303B_DEBUG
			LOG_W(__FUNCTION__, "Disalbe global interrupt failed [0x%02X]", data_read[0]);
		#endif
		}
	}
	FUSB303B_Delay(150);
	FUSB303B_Check(&status);
	if (status.type == DETECT_TYPE_SINK) {
	#if __FUSB303B_DEBUG
		LOG_D(__FUNCTION__, "Board may be powered by JUSB");
	#endif
		s_powerd_by_jusb = 1;
	}

	/* Clean all interrupt flag */
	data_write[0] = 0x14;
	data_write[1] = 0x7F;
	data_write[1] = 0x7F;
	FUSB303B_Write(data_write, 3);

	/* Enable global interrupt */
	FUSB303B_Read(REG_CONTROL, data_read, 1);
	if (data_read[0] & REG_CONTROL_INT_MASK) {
		data_write[0] = REG_CONTROL;
		data_write[1] = data_read[0] & (~REG_CONTROL_INT_MASK);
		FUSB303B_Write(data_write, 2);
		FUSB303B_Read(REG_CONTROL, data_read, 1);
		if (data_read[0] & REG_CONTROL_INT_MASK) {
		#if __FUSB303B_DEBUG
			LOG_W(__FUNCTION__, "Enable global interrupt failed [0x%02X]", data_read[0]);
		#endif
		}
	}

	/* Clean all interrupt flag */
	data_write[0] = 0x14;
	data_write[1] = 0x7F;
	data_write[1] = 0x7F;
	FUSB303B_Write(data_write, 3);

	return 0;
}

uint32_t FUSB303B_SetAutoPortrole(FUSB303B_AutoPortroleEnum role, bool en_audioacc)
{
	uint8_t data_read, t;
	uint8_t data_write[2];

	bool fail = false;

	switch (role) {
	case AUTO_PORTROLE_DRP:
		t = 0x04;
		break;
	case AUTO_PORTROLE_DRP_ONLY_SNK:
		t = 0x14;
		break;
	case AUTO_PORTROLE_DRP_ONLY_SRC:
		t = 0x24;
		break;
	case AUTO_PORTROLE_DRP_DONT_SAME_TIME:
		t = 0x34;
		break;
	case AUTO_PORTROLE_ONLY_SNK:
		t = 0x02;
		break;
	case AUTO_PORTROLE_ONLY_SRC:
		t = 0x01;
		break;
	default:
		fail = true;
		break;
	}

	if (fail) {
		return 1;
	}

	if (en_audioacc) {
		t |= 0x08;
	}

	FUSB303B_Read(REG_PORTROLE, &data_read, 1);
	data_read &= 0xC0;
	data_write[0] = REG_PORTROLE;
	data_write[1] = data_read | t;
	FUSB303B_Write(data_write, 2);
	FUSB303B_Read(REG_PORTROLE, &data_read, 1);
	if (data_read != data_write[1]) {
	#if __FUSB303B_DEBUG
		LOG_W(__FUNCTION__, "Write valid failed");
	#endif
		return 1;
	}

	data_write[0] = REG_MANUAL;
	data_write[1] = 0x00;
	FUSB303B_Write(data_write, 2);

	return 0;
}

uint32_t FUSB303B_SetCurrentLevel(FUSB303B_CurrentLevel level)
{
	uint8_t data_read, t;
	uint8_t data_write[2];

	bool fail = false;

	switch (level) {
	case CURRENT_LEVEL_DEFAULT:
		t = 0x02;
		break;
	case CURRENT_LEVEL_1_DOT_5_A:
		t = 0x04;
		break;
	case CURRENT_LEVEL_3_DOT_0_A:
		t = 0x06;
		break;
	default:
		fail = true;
		break;
	}

	if (fail) {
		return 1;
	}

	FUSB303B_Read(REG_CONTROL, &data_read, 1);
	data_read &= 0xF9;
	data_write[0] = REG_CONTROL;
	data_write[1] = data_read | t;
	FUSB303B_Write(data_write, 2);
	FUSB303B_Read(REG_CONTROL, &data_read, 1);
	if (data_read != data_write[1]) {
	#if __FUSB303B_DEBUG
		LOG_W(__FUNCTION__, "Write valid failed");
	#endif
		return 1;
	}

	return 0;
}

uint32_t FUSB303B_SetManualControl(FUSB303B_ManualControlEnum ctrl)
{
	uint8_t data_read;
	uint8_t data_write[2];

	switch (ctrl) {
	case MANUAL_CONTROL_SRC:
		data_write[1] = 0x20;
		break;
	case MANUAL_CONTROL_SNK:
		data_write[1] = 0x10;
		break;
	case MANUAL_CONTROL_UNATTACHED_SNK:
		data_write[1] = 0x08;
		break;
	case MANUAL_CONTROL_UNATTACHED_SRC:
		data_write[1] = 0x04;
		break;
	case MANUAL_CONTROL_DISABLED:
		data_write[1] = 0x02;
		break;
	case MANUAL_CONTROL_ERROR_RECOVERY:
		data_write[1] = 0x01;
		break;
	default:
		break;
	}

	data_write[0] = REG_MANUAL;
	FUSB303B_Write(data_write, 2);

	FUSB303B_Read(REG_MANUAL, &data_read, 1);
	data_write[0] = REG_PORTROLE;
	data_write[1] = data_read & 0xC0;
	FUSB303B_Write(data_write, 2);

	return 0;
}

/* ================ The following code is the porting layer ================ */
#include "hal_data.h"

#ifndef FUSB303B_EN_PIN
#define FUSB303B_EN_PIN			BSP_IO_PORT_06_PIN_06
#endif

#ifndef FUSB303B_IIC_INSTANCE
#define FUSB303B_IIC_INSTANCE	g_i2c_master0
#endif

#ifndef FUSB303B_IIC_CALLBACK
#define FUSB303B_IIC_CALLBACK	IIC0_Master_Callback
#endif

#define IIC_WAIT_DONE(x) do { R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS); } while (x == 0)

static uint8_t s_rx_done;
static uint8_t s_tx_done;

void FUSB303B_Delay(uint32_t ms)
{
	R_BSP_SoftwareDelay(ms, BSP_DELAY_UNITS_MILLISECONDS);
}

void FUSB303B_ENPinControl(uint8_t level)
{
	if (level) {
		R_BSP_PinWrite(FUSB303B_EN_PIN, BSP_IO_LEVEL_HIGH);
	}
	else {
		R_BSP_PinWrite(FUSB303B_EN_PIN, BSP_IO_LEVEL_LOW);
	}
}

void FUSB303B_Read(uint8_t addr, uint8_t *data, uint8_t length)
{
	uint8_t data_write = addr;

	R_IIC_MASTER_SlaveAddressSet(FUSB303B_IIC_INSTANCE.p_ctrl, FUSB303B_ADDR, I2C_MASTER_ADDR_MODE_7BIT);
	s_tx_done = 0;
	R_IIC_MASTER_Write(FUSB303B_IIC_INSTANCE.p_ctrl, &data_write, 1, true);
	IIC_WAIT_DONE(s_tx_done);
	s_rx_done = 0;
	R_IIC_MASTER_Read(FUSB303B_IIC_INSTANCE.p_ctrl, data, length, false);
	IIC_WAIT_DONE(s_rx_done);
}

void FUSB303B_Write(uint8_t *data, uint8_t length)
{
	R_IIC_MASTER_SlaveAddressSet(FUSB303B_IIC_INSTANCE.p_ctrl, FUSB303B_ADDR, I2C_MASTER_ADDR_MODE_7BIT);
	s_tx_done = 0;
	R_IIC_MASTER_Write(FUSB303B_IIC_INSTANCE.p_ctrl, data, length, true);
	IIC_WAIT_DONE(s_tx_done);
}

void FUSB303B_IIC_CALLBACK(i2c_master_callback_args_t *p_args)
{
	switch (p_args->event) {
	case I2C_MASTER_EVENT_ABORTED:
		break;
	case I2C_MASTER_EVENT_RX_COMPLETE:
		s_rx_done = 1;
		break;
	case I2C_MASTER_EVENT_TX_COMPLETE:
		s_tx_done = 1;
		break;
	case I2C_MASTER_EVENT_START:
		break;
	case I2C_MASTER_EVENT_BYTE_ACK:
		break;
	default:
		break;
	}
}
