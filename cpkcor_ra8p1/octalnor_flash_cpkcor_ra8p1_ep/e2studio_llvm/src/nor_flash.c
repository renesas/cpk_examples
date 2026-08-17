#include <stdio.h>
#include "hal_data.h"
#include "nor_flash.h"
#include "utils/log.h"

#define OSPI_NAME	g_nor_flash

#define JEDEC_MANUFACTURER_WINBOND	0xEF
#define W35N01JW_DEVICE_ID_L		0x21
#define W35N01JW_DEVICE_ID_H		0xDC
#define W35T51NW_MEMORY_TYPE		0x5B
#define W35T51NW_MEMORY_CAPACITY	0x1A
#define W35T51NW_EXTENSION			0x02

#define REPORT_ERR(r, e, msg, ...)	do { if (e) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); if (r) return e; } } while(0)
#define UNLIKE(e, r)				do { if (e != r) { return e; } } while (0)

struct NorFlash {
	uint8_t read_dummy_cycles_opi;
	uint32_t capacity;
	NorFlashChip chip;
	spi_flash_cfg_t cfg;
	ospi_b_extended_cfg_t cfg_ext;
};

const uint32_t gc_autocalibration[] = {
	0xFFFF0000,
	0x000800FF,
	0x00FFF700,
	0xF700F708
};

static struct NorFlash s_flash;

static const spi_flash_erase_command_t sc_erase_cmd_spi_w35n01jw[] = {
	{ .command = 0xD8, .size = 1024 * 256},
};

uint32_t NorFlash_EraseChip(void)
{
	uint32_t err;
	spi_flash_direct_transfer_t cmd;

	memset(&cmd, 0, sizeof(spi_flash_direct_transfer_t));

	err = NorFlash_SetWriteEnable();
	REPORT_ERR(1, err, "SetWriteEnable failed: 0x%X", err);

	if (g_nor_flash_ctrl.spi_protocol == SPI_FLASH_PROTOCOL_EXTENDED_SPI) {
		cmd.command = 0x60;
		cmd.command_length = 0x01;
	}
	else {
		cmd.command = 0x6060;
		cmd.command_length = 0x02;
	}
	err = R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	REPORT_ERR(1, err, "DirectTransfer failed: 0x%X", err);

	return 0;
}

uint32_t NorFlash_EraseSector(uint32_t sector)
{
	uint32_t err;
	spi_flash_direct_transfer_t cmd;

	err = NorFlash_SetWriteEnable();
	REPORT_ERR(1, err, "SetWriteEnable failed: 0x%X", err);

	memset(&cmd, 0, sizeof(spi_flash_direct_transfer_t));
	cmd.address = sector * NORFLASH_SECTOR_SIZE;
	cmd.address_length = 0x04;
	if (g_nor_flash_ctrl.spi_protocol == SPI_FLASH_PROTOCOL_EXTENDED_SPI) {
		cmd.command = 0x21;
		cmd.command_length = 0x01;
	}
	else {
		cmd.command = 0x2121;
		cmd.command_length = 0x02;
	}
	err = R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	REPORT_ERR(1, err, "DirectTransfer failed: 0x%X", err);
	err = NorFlash_WaitOperation(20);
	REPORT_ERR(1, err, "SetWriteEnable failed: 0x%X", err);

	return 0;
}

uint32_t NorFlash_Init(void)
{
	uint8_t i8;
	uint32_t expect;
	uint32_t cali[4];
	bsp_octaclk_settings_t octaclk;
	spi_flash_direct_transfer_t cmd;

	uint32_t err = 0;
	const ospi_b_extended_cfg_t * const p_cfg_extend = g_nor_flash_cfg.p_extend;
	const ospi_b_xspi_command_set_t *p_cmd_table = p_cfg_extend->p_xspi_command_set->p_table;
	uint8_t *p8 = p_cfg_extend->p_autocalibration_preamble_pattern_addr;

#if BSP_CFG_DCACHE_ENABLED
	SCB_DisableDCache();
#endif

	memset(&s_flash, 0, sizeof(s_flash));
	memcpy(&s_flash.cfg, &g_nor_flash_cfg, sizeof(spi_flash_cfg_t));
	memcpy(&s_flash.cfg_ext, g_nor_flash_cfg.p_extend, sizeof(ospi_b_extended_cfg_t));
	s_flash.chip = CHIP_UNKNOW;

	R_OSPI_B_Open(g_nor_flash.p_ctrl, g_nor_flash.p_cfg);
	R_OSPI_B_SpiProtocolSet(g_nor_flash.p_ctrl, SPI_FLASH_PROTOCOL_EXTENDED_SPI);

#if 1
	R_XSPI1->LIOCTL_b.RSTCS0 = 0;
	R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
	R_XSPI1->LIOCTL_b.RSTCS0 = 1;
	R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_MICROSECONDS);
#else
	uint32_t cfg = IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_DRIVE_HIGH | IOPORT_CFG_PORT_OUTPUT_HIGH;
	R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_12_PIN_07, cfg);
	R_BSP_PinWrite(BSP_IO_PORT_12_PIN_07, BSP_IO_LEVEL_LOW);
	R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
	R_BSP_PinWrite(BSP_IO_PORT_12_PIN_07, BSP_IO_LEVEL_HIGH);
	R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
#endif

	for (i8 = 0; i8 < p_cfg_extend->p_xspi_command_set->length; i8++) {
		if (p_cmd_table[i8].protocol == SPI_FLASH_PROTOCOL_8D_8D_8D) {
			break;
		}
	}
	if (i8 == p_cfg_extend->p_xspi_command_set->length) {
		LOG_W(__FUNCTION__, "Can't find a command table's protoal is 8D-8D-8D");
		s_flash.read_dummy_cycles_opi = 0x10;
	}
	else {
		s_flash.read_dummy_cycles_opi = p_cmd_table[i8].read_dummy_cycles;
		LOG_D(__FUNCTION__, "Set 8D-8D-8D read dummy cycles: %u", s_flash.read_dummy_cycles_opi);
	}

	cmd.address = 0x00;
	cmd.address_length = 0x00;
	cmd.command = 0x9F;
	cmd.command_length = 0x01;
	cmd.data = 0x00;
	cmd.data_length = 0x04;
	cmd.dummy_cycles = 0x00;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	expect = W35N01JW_DEVICE_ID_L;
	expect = (expect << 0x08) | W35N01JW_DEVICE_ID_H;
	expect = (expect << 0x08) | JEDEC_MANUFACTURER_WINBOND;
	expect = (expect << 0x08) | 0xFF;
	if (cmd.data == expect) {
		LOG_D(__FUNCTION__, "Chip: W35N01JW");
		s_flash.chip = CHIP_W35N01JW;
		s_flash.capacity = 1024 * 1024 * 128;
		s_flash.cfg.erase_command_list_length = 0x01;
		s_flash.cfg.p_erase_command_list = sc_erase_cmd_spi_w35n01jw;
		goto CHECK_AUTO_CALI;
	}
	expect = W35T51NW_EXTENSION;
	expect = (expect << 0x08) | W35T51NW_MEMORY_CAPACITY;
	expect = (expect << 0x08) | W35T51NW_MEMORY_TYPE;
	expect = (expect << 0x08) | JEDEC_MANUFACTURER_WINBOND;
	if (cmd.data == expect) {
		LOG_D(__FUNCTION__, "Chip: W35T51NW");
		s_flash.chip = CHIP_W35T51NW;
		s_flash.capacity = 1024 * 1024 * 64;
		goto CHECK_ADDR_MODE;
	}

	LOG_E(__FUNCTION__, "Unsupport NorFlash: 0x%X", cmd.data);
	return FSP_ERR_UNSUPPORTED;

CHECK_ADDR_MODE:
	/* 检查地址模式 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.command = 0x70;
	cmd.command_length = 0x01;
	cmd.data_length = 0x01;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	/* 如果在 3 字节地址模式就使用指令进入 4 字节地址模式 */
	if ((cmd.data & 0x01) == 0x00) {
		cmd.command = 0xB7;
		cmd.data_length = 0x00;
		R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
		R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MICROSECONDS);

		cmd.command = 0x70;
		cmd.data_length = 0x01;
		R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
		if ((cmd.data & 0x01) == 0x00) {
			LOG_W(__FUNCTION__, "Enter 4-byte address mode failed");
		}
		else {
			LOG_D(__FUNCTION__, "Enter 4-byte address mode");
		}
	}

CHECK_AUTO_CALI:
	LOG_D(__FUNCTION__, "Cali address: %p", p8);
	memcpy(cali, p8, 16);
	LOG_D(__FUNCTION__, "cali[0]: 0x%08X", cali[0]);
	LOG_D(__FUNCTION__, "cali[1]: 0x%08X", cali[1]);
	LOG_D(__FUNCTION__, "cali[2]: 0x%08X", cali[2]);
	LOG_D(__FUNCTION__, "cali[3]: 0x%08X", cali[3]);
	memset(cali, 0, 16);

	if (memcmp(p8, gc_autocalibration, sizeof(gc_autocalibration))) {
		LOG_D(__FUNCTION__, "Write autocalibration");
		R_OSPI_B_Erase(g_nor_flash.p_ctrl, p8, 4096);
		NorFlash_WaitOperation(200);
		R_OSPI_B_Write(g_nor_flash.p_ctrl, (uint8_t *)gc_autocalibration, p8, sizeof(gc_autocalibration));
		NorFlash_WaitOperation(200);
	}

	cmd.address = 0x00;
	cmd.address_length = 0x04;
	cmd.command = 0x85;
	cmd.data = 0x00;
	cmd.data_length = 0x01;
	cmd.dummy_cycles = 0x08;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-IOC : 0x%X", cmd.data);
	cmd.address = 0x01;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-DC  : 0x%X", cmd.data);
	cmd.address = 0x02;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-VLB : 0x%X", cmd.data);
	cmd.address = 0x03;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-DS  : 0x%X", cmd.data);
	cmd.address = 0x04;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-CRC : 0x%X", cmd.data);
	cmd.address = 0x05;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-AM  : 0x%X", cmd.data);
	cmd.address = 0x06;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-XIP : 0x%X", cmd.data);
	cmd.address = 0x07;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-Wrap: 0x%X", cmd.data);

	/* 设置 Dummy cycle */
	NorFlash_SetWriteEnable();
	memset(&cmd, 0, sizeof(cmd));
	cmd.address = 0x01;
	cmd.address_length = 0x04;
	cmd.command = 0x81;
	cmd.command_length = 0x01;
	cmd.data = s_flash.read_dummy_cycles_opi;
	cmd.data_length = 0x01;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);

	/* 设置 VCR-IOC = 0xE7 进入 Octal DDR 模式 */
	NorFlash_SetWriteEnable();
	memset(&cmd, 0, sizeof(cmd));
	cmd.address_length = 0x04;
	cmd.command = 0x81;
	cmd.command_length = 0x01;
	cmd.data = 0xE7;
	cmd.data_length = 0x01;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	octaclk.divider = BSP_CLOCKS_OCTA_CLOCK_DIV_1;
	octaclk.source_clock = BSP_CLOCKS_SOURCE_CLOCK_PLL2Q;
	R_BSP_OctaclkUpdate(&octaclk);
	err = R_OSPI_B_SpiProtocolSet(g_nor_flash.p_ctrl, SPI_FLASH_PROTOCOL_8D_8D_8D);
	if (err) {
		LOG_E(__FUNCTION__, "R_OSPI_B_SpiProtocolSet failed: %u", err);
	}
	else {
		LOG_D(__FUNCTION__, "Enter ODDR mode");
	}
	memset(cali, 0, 16);

	cmd.address = 0x00;
	cmd.address_length = 0x04;
	cmd.command = 0x8585;
	cmd.command_length = 0x02;
	cmd.data = 0x00;
	cmd.data_length = 0x02;
	cmd.dummy_cycles = 0x1F;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-IOC : 0x%X", cmd.data);
	cmd.address = 0x01;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-DC  : 0x%X", cmd.data);
	cmd.address = 0x02;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-VLB : 0x%X", cmd.data);
	cmd.address = 0x03;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-DS  : 0x%X", cmd.data);
	cmd.address = 0x04;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-CRC : 0x%X", cmd.data);
	cmd.address = 0x05;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-AM  : 0x%X", cmd.data);
	cmd.address = 0x06;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-XIP : 0x%X", cmd.data);
	cmd.address = 0x07;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	LOG_D(__FUNCTION__, "VCR-Wrap: 0x%X", cmd.data);
	__DSB();
	__ISB();

	SCB_InvalidateDCache();
#if 0
	memset(&cmd, 0, sizeof(cmd));
	cmd.address = (uint32_t)p8 - NORFLASH_MAP_START_ADDR;
	cmd.address_length = 0x04;
	cmd.command = 0x7C7C;
	cmd.command_length = 0x02;
	cmd.data_length = 0x04;
	cmd.dummy_cycles = s_read_dummy_cycles;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	cali[0] = cmd.data;
	cmd.address += 0x04;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	cali[1] = cmd.data;
	cmd.address += 0x04;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	cali[2] = cmd.data;
	cmd.address += 0x04;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	cali[3] = cmd.data;
#else
	memcpy(cali, p8, 16);
#endif
	LOG_D(__FUNCTION__, "cali[0]: 0x%08X", cali[0]);
	LOG_D(__FUNCTION__, "cali[1]: 0x%08X", cali[1]);
	LOG_D(__FUNCTION__, "cali[2]: 0x%08X", cali[2]);
	LOG_D(__FUNCTION__, "cali[3]: 0x%08X", cali[3]);

#if 0
	cmd.address = 0x2000;
	cmd.address_length = 0x04;
	cmd.command = 0x1212;
	cmd.data_u64 = 0x122355AA55BBCCDD;
	cmd.data_length = 0x08;
	cmd.dummy_cycles = 0x00;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);

	cmd.command = 0x0C0C;
	cmd.data_u64 = 0x00;
	cmd.dummy_cycles = 0x08;
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	if (cmd.data_u64 != 0x122355AA55BBCCDD) {
		LOG_W(__FUNCTION__, "Read not equal to write");
	}
#endif

#if BSP_CFG_DCACHE_ENABLED
	__DSB();
	__ISB();
	SCB_EnableDCache();
#endif

	return err;
}

uint32_t NorFlash_Read(uint32_t address, void *data, uint32_t length)
{
	uint32_t i, err;
	spi_flash_direct_transfer_t cmd;

	uint32_t remain = length % 8;
	uint64_t *p64 = (uint64_t *)data;

	memset(&cmd, 0, sizeof(spi_flash_direct_transfer_t));
	cmd.address = address;
	cmd.address_length = 0x04;
	cmd.data_length = 0x08;
	if (g_nor_flash_ctrl.spi_protocol == SPI_FLASH_PROTOCOL_EXTENDED_SPI) {
		cmd.command = 0x0C;
		cmd.command_length = 0x01;
		cmd.dummy_cycles = 0x08;
	}
	else {
		cmd.command = 0x7C7C;
		cmd.command_length = 0x02;
		cmd.dummy_cycles = s_flash.read_dummy_cycles_opi;
	}

	for (i = 0; i < (length / 8); i++) {
		err = R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
		REPORT_ERR(1, err, "DirectTransfer failed: %u, No.%u", err, i);
		p64[i] = cmd.data_u64;
		cmd.address += 8;
	}

	if (remain) {
		cmd.data_length = (uint8_t)remain;
		err = R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
		REPORT_ERR(1, err, "DirectTransfer failed: %u while process remain", err);
		memcpy(&p64[i], &cmd.data, remain);
	}

	return 0;
}

uint32_t NorFlash_SetWriteEnable(void)
{
	spi_flash_status_t status;
	spi_flash_direct_transfer_t cmd_we;
	spi_flash_direct_transfer_t cmd_rs;

	uint16_t r_cnt = 0;
	uint16_t w_cnt = 0;
	uint32_t err = 0;

	memset(&cmd_we, 0, sizeof(spi_flash_direct_transfer_t));
	memset(&cmd_rs, 0, sizeof(spi_flash_direct_transfer_t));
	if (g_nor_flash_ctrl.spi_protocol == SPI_FLASH_PROTOCOL_EXTENDED_SPI) {
		cmd_we.command = 0x06;
		cmd_we.command_length = 0x01;
		cmd_rs.command = 0x05;
		cmd_rs.command_length = 0x01;
		cmd_rs.data_length = 0x01;
	}
	else {
		cmd_we.command = 0x0606;
		cmd_we.command_length = 0x02;
		cmd_rs.command = 0x0505;
		cmd_rs.command_length = 0x02;
		cmd_rs.data_length = 0x02;
		cmd_rs.dummy_cycles = 0x08;
	}
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd_we, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	R_OSPI_B_StatusGet(g_nor_flash.p_ctrl, &status);
	r_cnt = 100;
	while (status.write_in_progress && r_cnt) {
		R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
		R_OSPI_B_StatusGet(g_nor_flash.p_ctrl, &status);
		r_cnt--;
	}
	if (r_cnt == 0) {
		LOG_E(__FUNCTION__, "Wait OSPI timeout");
		return FSP_ERR_TIMEOUT;
	}
	r_cnt = 0;

	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd_rs, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	while ((cmd_rs.data & 0x02) != 0x02) {
		R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
		R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd_rs, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
		r_cnt++;

		if ((r_cnt == 50) && ((cmd_rs.data & 0x02) != 0x02)) {
			r_cnt = 0;
			R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd_we, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
			w_cnt++;
		}

		if (w_cnt == 1200) {
			err = FSP_ERR_TIMEOUT;
			break;
		}
	}

	return err;
}

uint32_t NorFlash_WaitOperation(uint32_t timeout)
{
	spi_flash_direct_transfer_t cmd;

	(void)timeout;

#if 0
	spi_flash_status_t status;
	R_OSPI_B_StatusGet(g_nor_flash.p_ctrl, &status);
	while (status.write_in_progress) {
		if (timeout) {
			R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
			R_OSPI_B_StatusGet(g_nor_flash.p_ctrl, &status);
			timeout--;
		}
		else {
			return FSP_ERR_TIMEOUT;
		}
	}
#endif

	memset(&cmd, 0, sizeof(spi_flash_direct_transfer_t));
	if (g_nor_flash_ctrl.spi_protocol == SPI_FLASH_PROTOCOL_EXTENDED_SPI) {
		cmd.command = 0x05;
		cmd.command_length = 0x01;
		cmd.data_length = 0x01;
	}
	else {
		cmd.command = 0x0505;
		cmd.command_length = 0x02;
		cmd.data_length = 0x02;
		cmd.dummy_cycles = 0x08;
	}
#if 0
	while (timeout) {
		R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
		if ((cmd.data & 0x01) == 0x00) {
			return 0;
		}
		else {
			R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
			timeout--;
		}
	}

	return FSP_ERR_TIMEOUT;
#else
	R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	while (cmd.data & 0x01) {
		__NOP();
		R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	}

	return 0;
#endif
}

uint32_t NorFlash_WriteSector(uint32_t sector, void *data, uint32_t length)
{
	uint32_t i;
	uint32_t err;
	spi_flash_direct_transfer_t cmd;

	uint32_t remain = length % 8;
	uint64_t *p64 = (uint64_t *)data;

	err = NorFlash_EraseSector(sector);
	REPORT_ERR(1, err, "EraseSector failed");
	memset(&cmd, 0, sizeof(spi_flash_direct_transfer_t));
	cmd.address = sector * NORFLASH_SECTOR_SIZE;
	cmd.address_length = 0x04;
	cmd.data_length = 0x08;
	if (g_nor_flash_ctrl.spi_protocol == SPI_FLASH_PROTOCOL_EXTENDED_SPI) {
		cmd.command = 0x12;
		cmd.command_length = 0x01;
	}
	else {
		cmd.command = 0x1212;
		cmd.command_length = 0x02;
	}
	for (i = 0; i < (length / 8); i++) {
		cmd.data_u64 = p64[i];
		err = NorFlash_SetWriteEnable();
		REPORT_ERR(1, err, "SetWriteEnable failed");
		err = R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
		REPORT_ERR(1, err, "DirectTransfer failed");
		err = NorFlash_WaitOperation(20);
		REPORT_ERR(1, err, "WaitOperation failed");
		cmd.address += 8;
	}

	if (remain) {
		cmd.data_length = (uint8_t)remain;
		memcpy(&cmd.data, &p64[i], remain);
		err = NorFlash_SetWriteEnable();
		REPORT_ERR(1, err, "SetWriteEnable failed while process remain");
		err = R_OSPI_B_DirectTransfer(g_nor_flash.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
		REPORT_ERR(1, err, "DirectTransfer failed while process remain");
		err = NorFlash_WaitOperation(20);
		REPORT_ERR(1, err, "WaitOperation failed while process remain");
	}

	return 0;
}

void NorFlash_DumpOSPIReg(void)
{
	ospi_b_instance_ctrl_t *p_ctrl = (ospi_b_instance_ctrl_t *)OSPI_NAME.p_ctrl;

	printf("============== %s ==============\r\n", __FUNCTION__);
	printf("ABMCFG: 0x%08X\r\n", p_ctrl->p_reg->ABMCFG);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->BMCFGCH[0]);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->BMCFGCH[1]);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->BMCTL0);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->BMCTL1);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->CASTTCS[0]);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->CASTTCS[1]);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->CCCTLCS[0].CCCTL0);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->CCCTLCS[0].CCCTL1);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->CCCTLCS[0].CCCTL2);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->CCCTLCS[0].CCCTL3);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->CCCTLCS[0].CCCTL4);
	printf(": 0x%08X\r\n", p_ctrl->p_reg->CDCTL0);
}
