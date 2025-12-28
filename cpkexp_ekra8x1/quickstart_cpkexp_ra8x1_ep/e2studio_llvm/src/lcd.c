#include "common_utils.h"
#include "lcd.h"
#include "gimp.h"

#include "graphics/graphics.h"

#define PICTURE_ADDRESS_START       0x90002000
#define PICTURE_MAGIC		        0xbeefbeef

static uint32_t s_total_picture;
static uint8_t *pic_addrs[PICTURE_INDEX_MAX];
static uint8_t s_pictures[4 * 1024 * 1024] __attribute__((section(".sdram_noinit")));

void init_picture_info(void)
{
    spi_flash_direct_transfer_t command;
    uint32_t i;

	uint32_t *addr = (uint32_t *)PICTURE_ADDRESS_START;
	uint32_t *p32 = (uint32_t *)s_pictures;
	uint32_t cnt = 20;

	while (cnt) {
		if (addr[0] == PICTURE_MAGIC)
			break;
		printf("wait for OSPI stable for 100ms.\n");
		R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MICROSECONDS);
		cnt--;
	}

	if (cnt == 0) {
		printf("can not find vaild picture info on OSPI.\n");
		return;
	}

	s_total_picture = cnt = addr[1];
	addr += 2;

	command.address_length = 3;
	command.command = 0x03;
	command.command_length = 1;
	command.data = 0;
	command.data_length = 4;
	command.dummy_cycles = 0;
	for (i = 0; i < (1024 * 1024); i++) {
	    command.address = 0x2000 + i * 4;
	    R_OSPI_B_DirectTransfer(g_ospi0.p_ctrl, &command, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	    *p32 = command.data;
	    p32++;
	}

	for (i = 0; i < cnt; i++) {
		pic_addrs[i] = (uint8_t *)((uint32_t)&s_pictures + addr[i]);
		// printf("offset of pic%d is 0x%x\n", i, (uint32_t)pic_addrs[i]);
	}
}

uint16_t *lcd_get_pic_from_flash(uint8_t index)
{
	return (uint16_t *)pic_addrs[index];
}

uint32_t get_sub_image_data(st_image_data_t ref, uint32_t sub_image)
{
	(void)ref;
	if (sub_image >= s_total_picture) {
		printf("picture %d is not exist.\n", sub_image);
		return PICTURE_ADDRESS_START;
	} else {
		return (uint32_t)(unsigned long)pic_addrs[sub_image];
	}
}

uint32_t get_image_data(st_image_data_t ref)
{
	(void)ref;
	return 0x90001000;
}
