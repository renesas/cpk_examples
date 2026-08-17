#include <byteswap.h>

#include "hal_data.h"
#include "hyper_ram.h"
#include "utils/log.h"

#define HYPER_RAM_OPI_INSTANCE		g_ospi_hyper_ram
#define HYPER_RAM_CFG_REG_0_ADDRESS (0x01000000)
#define HYPER_RAM_CFG_REG_1_ADDRESS (0x01000001)

ospi_b_xspi_command_set_t g_hyper_ram_commands[] = {
    {
        .protocol = SPI_FLASH_PROTOCOL_8D_8D_8D,
        .frame_format = OSPI_B_FRAME_FORMAT_XSPI_PROFILE_2_EXTENDED,
        .latency_mode = OSPI_B_LATENCY_MODE_FIXED,
        .command_bytes = OSPI_B_COMMAND_BYTES_1,
        .address_bytes = SPI_FLASH_ADDRESS_BYTES_4,

        .read_command = 0xA0,
        .read_dummy_cycles = 0x05,
        .program_command = 0x20,
        .program_dummy_cycles = 0x05,

        .address_msb_mask = 0xF0,
        .status_needs_address = false,

        .p_erase_commands = NULL,
    }
};

ospi_b_xspi_command_set_t g_hyper_ram_erase_commands[] = {
    {
        .protocol = SPI_FLASH_PROTOCOL_8D_8D_8D,
        .frame_format = OSPI_B_FRAME_FORMAT_XSPI_PROFILE_2_EXTENDED,
        .latency_mode = OSPI_B_LATENCY_MODE_FIXED,
        .command_bytes = OSPI_B_COMMAND_BYTES_1,
        .address_bytes = SPI_FLASH_ADDRESS_BYTES_4,

        .read_command = 0xA0,
        .read_dummy_cycles = 0x05,
        .program_command = 0x20,
        .program_dummy_cycles = 0x05,

        .address_msb_mask = 0xF0,
        .status_needs_address = false,

        .p_erase_commands = NULL,
    }
};

void HyperRAM_Init(void)
{
	uint16_t val;
	spi_flash_direct_transfer_t cmd;

	uint32_t cfg = IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_DRIVE_HIGH | IOPORT_CFG_PORT_OUTPUT_HIGH;

	memset(&cmd, 0, sizeof(spi_flash_direct_transfer_t));
	R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_12_PIN_07, cfg);
	R_BSP_PinWrite(BSP_IO_PORT_12_PIN_07, BSP_IO_LEVEL_LOW);
	R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
	R_BSP_PinWrite(BSP_IO_PORT_12_PIN_07, BSP_IO_LEVEL_HIGH);
	R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);

	R_OSPI_B_Open(HYPER_RAM_OPI_INSTANCE.p_ctrl, HYPER_RAM_OPI_INSTANCE.p_cfg);
	R_OSPI_B_SpiProtocolSet(HYPER_RAM_OPI_INSTANCE.p_ctrl, SPI_FLASH_PROTOCOL_8D_8D_8D);
	R_XSPI1->LIOCFGCS_b[0].LATEMD = 1;

	cmd.address = HYPER_RAM_CFG_REG_0_ADDRESS;
	cmd.address_length = 0x04;
	cmd.command = 0xE000;
	cmd.command_length = 0x02;
	cmd.data_length = 0x02;
	cmd.dummy_cycles = 0x0B;
	R_OSPI_B_DirectTransfer(HYPER_RAM_OPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	val = (uint16_t)cmd.data;
	LOG_D(__FUNCTION__, "CR0: 0x%X", bswap_16(val));
	__DSB();
	__ISB();

	val = 0x8F05;
	LOG_D(__FUNCTION__, "Set CR0: 0x%X", val);
	cmd.command = 0x6000;
	cmd.data = bswap_16(val);
	cmd.dummy_cycles = 0x00;
	R_OSPI_B_DirectTransfer(HYPER_RAM_OPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	__DSB();
	__ISB();

	cmd.address = HYPER_RAM_CFG_REG_0_ADDRESS;
	cmd.command = 0xE000;
	cmd.data = 0x00;
	cmd.dummy_cycles = 0x0B;
	R_OSPI_B_DirectTransfer(HYPER_RAM_OPI_INSTANCE.p_ctrl, &cmd, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	val = (uint16_t)cmd.data;
	LOG_D(__FUNCTION__, "CR0: 0x%X", bswap_16(val));

	LOG_D(__FUNCTION__, "LIOCFGCS[0]: 0x%X", R_XSPI1->LIOCFGCS[0]);
}
