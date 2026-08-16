#include "hal_data.h"
#include "w25qxx.h"
#include "utils/util.h"

#define OSPI_INSTANCE	g_ospi_w25qxx
#define OSPI_CFG		UTIL_CONCAT(OSPI_INSTANCE, _cfg)
#define OSPI_CTRL		UTIL_CONCAT(OSPI_INSTANCE, _ctrl)

#ifndef __W25QXX_DEBUG
#define __W25QXX_DEBUG	0
#endif

#if __W25QXX_DEBUG
#include "utils/log.h"
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#endif

static void softReset(void);

uint32_t W25QXX_EraseChip(void)
{
	uint32_t err;
	spi_flash_direct_transfer_t cmd;

	err = W25QXX_WriteEnable(200);
	UNLIKE_RETURN(err, 0, "W25QXX_WriteEnable() failed");
	memset(&cmd, 0, sizeof(cmd));
	cmd.command = 0xC7;
	cmd.command_length = 0x01;
	R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);

	return 0;
}

uint32_t W25QXX_EraseSector(uint32_t sector)
{
	uint32_t err;
	spi_flash_direct_transfer_t cmd;

	spi_flash_protocol_t current = OSPI_CFG.spi_protocol;

	if (current == SPI_FLASH_PROTOCOL_1S_4S_4S) {
		R_OSPI_B_SpiProtocolSet(OSPI_INSTANCE.p_ctrl, SPI_FLASH_PROTOCOL_EXTENDED_SPI);
	}
	err = W25QXX_WriteEnable(100);
	UNLIKE_RETURN(err, 0, "W25QXX_WriteEnable() failed");
	memset(&cmd, 0, sizeof(cmd));
	cmd.address = sector * W25QXX_SECTOR_SIZE;
	cmd.address_length = 0x04;
	cmd.command = 0x21;
	cmd.command_length = 0x01;
	R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	if (current == SPI_FLASH_PROTOCOL_1S_4S_4S) {
		W25QXX_WaitOperation(200);
		R_OSPI_B_SpiProtocolSet(OSPI_INSTANCE.p_ctrl, current);
	}

	return err;
}

uint32_t W25QXX_GetID(uint8_t *manufacturer_id, uint8_t *device_id, uint64_t *unique_id)
{
	spi_flash_direct_transfer_t cmd;

	spi_flash_protocol_t current = OSPI_CTRL.spi_protocol;

	memset(&cmd, 0, sizeof(cmd));
	if ((manufacturer_id != NULL) || (device_id != NULL)) {
		cmd.command_length = 0x01;
		if (OSPI_CTRL.spi_protocol == SPI_FLASH_PROTOCOL_1S_1S_1S) {
			cmd.address_length = 0x04;
			cmd.command = 0x90;
			cmd.data_length = 0x02;
		}
		else {
			cmd.address_length = 0x04;
			cmd.command = 0x94;
			cmd.data_length = 0x04;
			cmd.dummy_cycles = 0x06;
		}
		R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);

		if (manufacturer_id != NULL) {
			*manufacturer_id = (uint8_t)(cmd.data >> 8);
		}

		if (device_id != NULL) {
			*device_id = cmd.data & 0x00FF;
		}
	}

	if (unique_id != NULL) {
		if (current == SPI_FLASH_PROTOCOL_1S_4S_4S) {
			R_OSPI_B_SpiProtocolSet(OSPI_INSTANCE.p_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);
			R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MICROSECONDS);
		}
		cmd.address_length = 0x04;
		cmd.command = 0x4B;
		cmd.command_length = 0x01;
		cmd.data_length = 0x08;
		R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
		*unique_id = cmd.data_u64;
		if (current == SPI_FLASH_PROTOCOL_1S_4S_4S) {
			R_OSPI_B_SpiProtocolSet(OSPI_INSTANCE.p_ctrl, current);
		}
	}

	return 0;
}

uint32_t W25QXX_Init(void)
{
	uint8_t manufacturer_id, manufacturer_id_1;
	uint8_t device_id, device_id_1;
	uint64_t unique_id, unique_id_1;
	spi_flash_direct_transfer_t cmd;

	R_OSPI_B_Open(OSPI_INSTANCE.p_ctrl, OSPI_INSTANCE.p_cfg);
	R_OSPI_B_SpiProtocolSet(OSPI_INSTANCE.p_ctrl, SPI_FLASH_PROTOCOL_EXTENDED_SPI);

	softReset();
	memset(&cmd, 0, sizeof(cmd));
	cmd.command = 0x15;
	cmd.command_length = 0x01;
	cmd.data_length = 0x01;
	R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	if ((cmd.data & 0x01) == 0x00) {
	#if __W25QXX_DEBUG
		LOG_D(__FUNCTION__, "Enter 4-byte address mode");
	#endif
		cmd.command = 0xB7;
		cmd.data_length = 0x00;
		R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
		cmd.command = 0x15;
		cmd.data_length = 0x01;
		R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
		if ((cmd.data & 0x01) == 0x00) {
		#if __W25QXX_DEBUG
			LOG_E(__FUNCTION__, "Enter 4-byte address mode failed, Status REG3: 0x%X", cmd.data);
		#endif
		}
	}
	else {
	#if __W25QXX_DEBUG
		LOG_D(__FUNCTION__, "Already in 4-byte address mode");
	#endif
	}

	/* Try a faster clock speed */
	W25QXX_GetID(&manufacturer_id, &device_id, &unique_id);
#if __W25QXX_DEBUG
	LOG_D(__FUNCTION__, "manufacturer_id: 0x%X", manufacturer_id);
	LOG_D(__FUNCTION__, "device_id: 0x%X", device_id);
	LOG_D(__FUNCTION__, "unique_id: 0x%llX", unique_id);
#endif
	if (manufacturer_id != 0xEF) {
	#if __W25QXX_DEBUG
		LOG_E(__FUNCTION__, "manufacturer_id error: 0x%X", manufacturer_id);
	#endif
		return FSP_ERR_UNSUPPORTED;
	}

	/* Switch to 1S-4S-4S */
	memset(&cmd, 0, sizeof(cmd));
	cmd.command = 0x35;
	cmd.command_length = 0x01;
	cmd.data_length = 0x01;
	R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	if ((cmd.data & 0x02) == 0) {
		cmd.command = 0x50;
		cmd.data_length = 0x00;
		R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);

		cmd.command = 0x31;
		cmd.data |= 0x02;
		cmd.data_length = 0x01;
		R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);

		cmd.command = 0x35;
		cmd.data = 0x00;
		R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);

		if ((cmd.data & 0x02) == 0) {
		#if __W25QXX_DEBUG
			LOG_E(__FUNCTION__, "Set QE failed");
		#endif
		}
		else {
		#if __W25QXX_DEBUG
			LOG_D(__FUNCTION__, "Set QE success");
		#endif
		}
	}
	else {
	#if __W25QXX_DEBUG
		LOG_D(__FUNCTION__, "QE already enable");
	#endif
	}

	R_OSPI_B_SpiProtocolSet(OSPI_INSTANCE.p_ctrl, SPI_FLASH_PROTOCOL_1S_4S_4S);
	W25QXX_GetID(&manufacturer_id_1, &device_id_1, &unique_id_1);
	if ((manufacturer_id != manufacturer_id_1) || (device_id != device_id_1) || (unique_id != unique_id_1)) {
	#if __W25QXX_DEBUG
		LOG_W(__FUNCTION__, "Read ID in 1S-4S-4S doesn't equal to 1S-1S-1S");
		LOG_W(__FUNCTION__, "manufacturer_id: 0x%X", manufacturer_id_1);
		LOG_W(__FUNCTION__, "device_id: 0x%X", device_id_1);
		LOG_W(__FUNCTION__, "unique_id: 0x%llX", unique_id_1);
	#endif
	}
#if __W25QXX_DEBUG
	LOG_D(__FUNCTION__, "Enter 1S-4S-4S mode.");
#endif

	return 0;
}

uint32_t W25QXX_WaitOperation(uint32_t timeout)
{
	spi_flash_direct_transfer_t cmd;

	spi_flash_protocol_t current = OSPI_CTRL.spi_protocol;

	if (current == SPI_FLASH_PROTOCOL_1S_4S_4S) {
		R_OSPI_B_SpiProtocolSet(&OSPI_CTRL, SPI_FLASH_PROTOCOL_1S_1S_1S);
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.command = 0x05;
	cmd.command_length = 0x01;
	cmd.data_length = 0x01;
	R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	while ((cmd.data & 0x01) && timeout) {
		R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
		timeout--;
		R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	}

	if (current == SPI_FLASH_PROTOCOL_1S_4S_4S) {
		R_OSPI_B_SpiProtocolSet(&OSPI_CTRL, SPI_FLASH_PROTOCOL_1S_4S_4S);
	}

	if (cmd.data & 0x01) {
	#if __W25QXX_DEBUG
		LOG_E(__FUNCTION__, "Timeout, last read: 0x%X", cmd.data);
	#endif
		return FSP_ERR_TIMEOUT;
	}

	return 0;
}

uint32_t W25QXX_Write(uint8_t * const src, uint8_t * const dest, uint32_t length, uint8_t erase)
{
	uint32_t i;
	uint32_t err;

	uint8_t *p_src = (uint8_t *)src;
	uint8_t *p_dest = (uint8_t *)dest;
	uint32_t repeat = length / 64;
	uint32_t remain = length % 64;
	spi_flash_protocol_t current = OSPI_CTRL.spi_protocol;

	if (((uint32_t)dest < W25QXX_MAP_START_ADDR) || ((uint32_t)dest > (W25QXX_MAP_START_ADDR + W25QXX_SIZE))) {
	#if __W25QXX_DEBUG
		LOG_E(__FUNCTION__, "dest out of range");
	#endif
		return FSP_ERR_OUT_OF_MEMORY;
	}

	if (current == SPI_FLASH_PROTOCOL_1S_4S_4S) {
		R_OSPI_B_SpiProtocolSet(OSPI_INSTANCE.p_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);
	}

	if (erase) {
		R_OSPI_B_Erase(OSPI_INSTANCE.p_ctrl, dest, length);
	}

	for (i = 0; i < repeat; i++) {
		err = R_OSPI_B_Write(OSPI_INSTANCE.p_ctrl, p_src, p_dest, 64);
		UNLIKE_RETURN(err, 0, "R_OSPI_B_Write() failed, err: %u, i: %u", err, i);
		err = W25QXX_WaitOperation(2000);
		UNLIKE_RETURN(err, 0, "WaitOperation() failed, err: %u, i: %u", err, i);
		p_src = &p_src[64];
		p_dest = &p_dest[64];
	}
	if (remain) {
		err = R_OSPI_B_Write(OSPI_INSTANCE.p_ctrl, p_src, p_dest, remain);
		UNLIKE_RETURN(err, 0, "R_OSPI_B_Write() failed, err: %u", err);
		err = W25QXX_WaitOperation(2000);
		UNLIKE_RETURN(err, 0, "WaitOperation() failed, err: %u", err);
	}

	if (current == SPI_FLASH_PROTOCOL_1S_4S_4S) {
		W25QXX_WaitOperation(200);
		R_OSPI_B_SpiProtocolSet(OSPI_INSTANCE.p_ctrl, current);
	}

	return err;
}

uint32_t W25QXX_WriteEnable(uint32_t timeout)
{
	spi_flash_direct_transfer_t cmd;

	spi_flash_protocol_t current = OSPI_CTRL.spi_protocol;

	if (current == SPI_FLASH_PROTOCOL_1S_4S_4S) {
		R_OSPI_B_SpiProtocolSet(&OSPI_CTRL, SPI_FLASH_PROTOCOL_1S_1S_1S);
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.command = 0x06;
	cmd.command_length = 0x01;
	R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	cmd.command = 0x05;
	cmd.data_length = 0x01;
	R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	while (((cmd.data & 0x02) == 0x00) && (timeout)) {
		R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
		timeout--;
		R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	}

	if (current == SPI_FLASH_PROTOCOL_1S_4S_4S) {
		R_OSPI_B_SpiProtocolSet(&OSPI_CTRL, SPI_FLASH_PROTOCOL_1S_4S_4S);
	}

	if ((cmd.data & 0x02) == 0x00) {
		return FSP_ERR_TIMEOUT;
	}

	return 0;
}

static void softReset(void)
{
	spi_flash_direct_transfer_t cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.command = 0x66;
	cmd.command_length = 0x01;
	R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
	cmd.command = 0x99;
	R_OSPI_B_DirectTransfer(OSPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MICROSECONDS);
}
