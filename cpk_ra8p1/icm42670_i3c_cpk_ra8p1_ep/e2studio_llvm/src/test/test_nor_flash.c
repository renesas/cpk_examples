#include "test.h"

#if TEST_EN_NOR_FLASH

#include <stdio.h>

#include "console.h"
#include "hal_data.h"
#include "nor_flash.h"
#include "perf_counter/perf_counter.h"
#include "utils/log.h"

#ifndef TEST_NOR_FLASH_EN_FULL_CHECK
#define TEST_NOR_FLASH_EN_FULL_CHECK	0
#endif

#define ENABLE_CHIP_ERASE	1
#define ENABLE_DIFF_CLK		0
#define ENABLE_VOLATILE		1

#define CHECK_RESULT(r, mp, mf)	do { if (r) { if (mf != NULL) { puts(mf); } return r; } if (mp != NULL) { puts(mp); } } while(0)

#define CACHE_SIZE		(1024 * 64)

#if ENABLE_VOLATILE
#define VOLATILE	volatile
#else
#define VOLATILE
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"

static VOLATILE uint8_t s_rcache[NORFLASH_SECTOR_SIZE];
static VOLATILE uint8_t s_cache[CACHE_SIZE] __attribute__((section(".dtcm_noinit")));
static VOLATILE uint8_t s_ram[CACHE_SIZE] __attribute__((section(".ram_noinit")));
static VOLATILE uint8_t s_ram_nc[CACHE_SIZE] __attribute__((section(".ram_noinit_nocache")));

#if TEST_NOR_FLASH_EN_FULL_CHECK
static uint8_t checkFullChipNoCache(uint32_t num_sector);
#else
static void checkSpeedWrite(void);
#endif
static void checkSpeedRead(void);

uint32_t TestNorFlash(uint32_t start_addr, uint32_t size)
{
	uint8_t result;

	(void)start_addr;
	(void)size;

	if (SCB->CCR & SCB_CCR_DC_Msk) {
		SCB_DisableDCache();
		puts("Turn off DCache");
	}
	else {
		puts("DCache already disable");
	}

#if TEST_NOR_FLASH_EN_FULL_CHECK
	result = checkFullChipNoCache(NORFLASH_SIZE / NORFLASH_SECTOR_SIZE);
	CHECK_RESULT(result, "Full chip check PASS", "Full chip check FAILED");
#else
	checkSpeedWrite();
#endif

	checkSpeedRead();

	return 0;
}

#if TEST_NOR_FLASH_EN_FULL_CHECK
static uint8_t checkFullChipNoCache(uint32_t num_sector)
{
	uint8_t bit_num;
	uint8_t i8;
	uint32_t err;
	uint32_t i, j, t;
	uint32_t val_r;
	uint32_t mask;
	int64_t time_start, time_end;
	int64_t time_erase_start;
#if ENABLE_DIFF_CLK
	bsp_octaclk_settings_t octaclk;
#endif

	float speed = 0.0f;
	float speed_sum = 0.0f;
	int64_t time_erase_sum = 0;
	int64_t time_write_sum = 0;
	uint32_t mask_src = 0xA5A5A5A5;
	VOLATILE uint32_t *p32_e = NULL;
	VOLATILE uint32_t *p32_w = (VOLATILE uint32_t *)s_ram;
	VOLATILE uint32_t *p32_r = (VOLATILE uint32_t *)NORFLASH_MAP_START_ADDR;

	memset((void *)s_ram, 0, NORFLASH_SECTOR_SIZE);
#if ENABLE_DIFF_CLK
	octaclk.source_clock = BSP_CLOCKS_SOURCE_CLOCK_PLL2Q;
#endif
#if ENABLE_CHIP_ERASE
	puts("Erase fullchip will take about 80 seconds, please wait");
	time_start = get_system_ms();
	NorFlash_EraseChip();
	NorFlash_WaitOperation(40000000);
	time_end = get_system_ms();
	printf("Erase chip using time: %llu ms\r\n", time_end - time_start);
#endif
	puts("Check fullchip will take long time, you can input any character to interrupt this test");
	for (j = 0; j < num_sector; j++) {
		if ((j % 1024) == 0) {
			printf("Check sector: %u ~ %u\r\n", j, j + 1023);
		}

		p32_e = (VOLATILE uint32_t *)(NORFLASH_MAP_START_ADDR + j * NORFLASH_SECTOR_SIZE);
		for (i = 0; i < (NORFLASH_SECTOR_SIZE / 4); i++) {
			/* 计算 i 的有效位数 */
			bit_num = 0;
			t = i + j * NORFLASH_SECTOR_SIZE / 4;
			while (t) {
				t >>= 0x01;
				bit_num++;
			}
			/* 按字节调整 */
			if (bit_num < 8) {
				bit_num = 8;
			}
			else if ((bit_num > 8) && (bit_num < 16)) {
				bit_num = 16;
			}
			else if ((bit_num > 16) && (bit_num < 24)) {
				bit_num = 24;
			}
			else if ((bit_num > 24) && (bit_num < 32)) {
				bit_num = 32;
			}
			/* 通过 i 的有效位数调整掩码，i 的高位写入 mask_src */
			mask = 0xFFFFFFFF;
			for (i8 = 0; i8 < bit_num; i8++) {
				mask <<= 0x01;
			}
			p32_w[i] = ((mask & mask_src) | (i + j * NORFLASH_SECTOR_SIZE / 4));
		}

	#if ENABLE_DIFF_CLK
		octaclk.divider = BSP_CLOCKS_OCTA_CLOCK_DIV_2;
		R_BSP_OctaclkUpdate(&octaclk);
	#endif
	#if ENABLE_CHIP_ERASE == 0
		time_erase_start = get_system_us();
		R_OSPI_B_Erase(g_nor_flash.p_ctrl, (uint8_t *)p32_e, NORFLASH_SECTOR_SIZE);
		NorFlash_WaitOperation(4000);
	#endif
		time_start = get_system_us();
	#if 0
		R_OSPI_B_Write(g_nor_flash.p_ctrl, (uint8_t *)p32_w, (uint8_t *)p32_e, NORFLASH_SECTOR_SIZE);
		NorFlash_WaitOperation(8000);
	#else
		VOLATILE uint32_t *p32_wcache = p32_w;
		VOLATILE uint32_t *p32_wtoflash = p32_e;
		for (t = 0; t < (NORFLASH_SECTOR_SIZE / 64); t++) {
			err = R_OSPI_B_Write(g_nor_flash.p_ctrl, (uint8_t *)p32_wcache, (uint8_t *)p32_wtoflash, 64);
			if (err) {
				printf("%s ==> R_OSPI_B_Write failed: %u\r\n", __FUNCTION__, err);
				printf("    t = %u\r\n", t);
				return 1;
			}
			err = NorFlash_WaitOperation(4000);
			if (err) {
				printf("%s ==> WaitOperation failed: %u\r\n", __FUNCTION__, err);
				printf("    t = %u\r\n", t);
				return 1;
			}
			p32_wcache = &p32_wcache[16];
			p32_wtoflash = &p32_wtoflash[16];
		}
	#endif
		/* 计算写入速度 */
		time_end = get_system_us();
		if (time_end != time_start) {
			speed = (float)NORFLASH_SECTOR_SIZE / (float)(time_end - time_start);
			speed = speed * 1000000 / 1024;
		}
		speed_sum += speed;
		/* 记录写入时间 */
		time_write_sum += (time_end - time_start);
	#if ENABLE_CHIP_ERASE == 0
		/* 记录擦除时间 */
		time_erase_sum += (time_start - time_erase_start);
	#endif

	#if ENABLE_DIFF_CLK
		octaclk.divider = BSP_CLOCKS_OCTA_CLOCK_DIV_1;
		R_BSP_OctaclkUpdate(&octaclk);
	#endif
	#if 1
		for (i = 0; i < (NORFLASH_SECTOR_SIZE / 4); i++) {
			// val_r = p32_r[j * NORFLASH_SECTOR_SIZE / 4 + i];
			val_r = *p32_r;
			if (val_r != p32_w[i]) {
				printf("%s ==> Failed at\r\n", __FUNCTION__);
				printf("    sector: %u No.%u\r\n", j, i);
				printf("    expect: 0x%X read: 0x%X\r\n", p32_w[i], val_r);
				return 1;
			}
			p32_r++;
		}
	#else
		NorFlash_Read(j * NORFLASH_SECTOR_SIZE, (void *)s_rcache, NORFLASH_SECTOR_SIZE);
		VOLATILE uint32_t *p32_rcache = (VOLATILE uint32_t *)s_rcache;
		for (i = 0; i < (NORFLASH_SECTOR_SIZE / 4); i++) {
			if (p32_rcache[i] != p32_w[i]) {
				printf("%s ==> Failed at\r\n", __FUNCTION__);
				printf("    sector: %u No.%u\r\n", j, i);
				printf("    expect: 0x%X read: 0x%X\r\n", p32_w[i], p32_rcache[i]);
				return 1;
			}
		}
	#endif
		if ((j % 1024) == 1023) {
			printf("Sector: %u ~ %u PASS\r\n", j - 1023, j);
		}

		if (CONSOLE_HasData()) {
			puts("Test early termination");
			printf("Total check sector: 0 ~ %u\r\n", j);
			while (CONSOLE_HasData()) { getchar(); }
			break;
		}
	}

#if ENABLE_CHIP_ERASE == 0
	printf("Average erase sector time: %llu us\r\n", time_erase_sum / j);
#endif
	printf("Average write time: %llu us, speed: %.2fKB/s\r\n", time_write_sum / j, speed_sum / (float)j);

	return 0;
}
#else
static void checkSpeedWrite(void)
{
	uint32_t i, t;
	int64_t time_start, time_end;

	float speed = 0.0f;
	VOLATILE uint32_t *p32_e = (VOLATILE uint32_t *)NORFLASH_MAP_START_ADDR;
	VOLATILE uint32_t *p32_w = (VOLATILE uint32_t *)s_ram;
	VOLATILE uint32_t *p32_wtoflash = p32_e;

	for (i = 0; i < NORFLASH_SECTOR_SIZE / 4; i++) {
		p32_w[i] = i;
	}

	R_OSPI_B_Erase(g_nor_flash.p_ctrl, (uint8_t *)p32_e, NORFLASH_SECTOR_SIZE);
	NorFlash_WaitOperation(4000);

	time_start = get_system_us();
	for (t = 0; t < (NORFLASH_SECTOR_SIZE / 64); t++) {
		R_OSPI_B_Write(g_nor_flash.p_ctrl, (uint8_t *)p32_w, (uint8_t *)p32_wtoflash, 64);
		NorFlash_WaitOperation(4000);
		p32_w = &p32_w[16];
		p32_wtoflash = &p32_wtoflash[16];
	}
	time_end = get_system_us();
	if (time_end != time_start) {
		speed = (float)NORFLASH_SECTOR_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024;
		printf("Write speed: %.2f KB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
}
#endif

static void checkSpeedRead(void)
{
	uint32_t i;
	fsp_err_t err;
	float speed;
	VOLATILE int64_t time_start, time_end;
	VOLATILE int64_t tick_start, tick_end;
	VOLATILE uint32_t reg;

	VOLATILE uint8_t *p8_dtcm = s_cache;
	VOLATILE uint8_t *p8_ram = s_ram;
	VOLATILE uint8_t *p8_ram_nc = s_ram_nc;
	VOLATILE uint8_t *p8_flash = (VOLATILE uint8_t *)NORFLASH_MAP_START_ADDR;
	VOLATILE uint16_t *p16_dtcm = (VOLATILE uint16_t *)s_cache;
	VOLATILE uint16_t *p16_ram = (VOLATILE uint16_t *)s_ram;
	VOLATILE uint16_t *p16_ram_nc = (VOLATILE uint16_t *)s_ram_nc;
	VOLATILE uint16_t *p16_flash = (VOLATILE uint16_t *)NORFLASH_MAP_START_ADDR;
	VOLATILE uint32_t *p32_dtcm = (VOLATILE uint32_t *)s_cache;
	VOLATILE uint32_t *p32_ram = (VOLATILE uint32_t *)s_ram;
	VOLATILE uint32_t *p32_ram_nc = (VOLATILE uint32_t *)s_ram_nc;
	VOLATILE uint32_t *p32_flash = (VOLATILE uint32_t *)NORFLASH_MAP_START_ADDR;
	VOLATILE uint64_t *p64_dtcm = (VOLATILE uint64_t *)s_cache;
	VOLATILE uint64_t *p64_ram = (VOLATILE uint64_t *)s_ram;
	VOLATILE uint64_t *p64_ram_nc = (VOLATILE uint64_t *)s_ram_nc;
	VOLATILE uint64_t *p64_flash = (VOLATILE uint64_t *)NORFLASH_MAP_START_ADDR;

#if BSP_CFG_DCACHE_ENABLED
	/* 关闭 forced write-through，进入 write-back 模式 */
	MEMSYSCTL->MSCR &= ~(MEMSYSCTL_MSCR_FORCEWT_Msk);
	__DSB();
	__ISB();
	reg = MEMSYSCTL->MSCR;
	if (reg & MEMSYSCTL_MSCR_FORCEWT_Msk) {
		puts("Set write-back failed");
		return;
	}
	SCB_EnableDCache();
	puts("Turn on DCache, write-back mode");
#endif

	/* Flash -> DTCM，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_dtcm[i] = p8_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to DTCM                  with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> DTCM，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_dtcm[i] = p16_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to DTCM                  with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> DTCM，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_dtcm[i] = p32_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to DTCM                  with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> DTCM，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_dtcm[i] = p64_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to DTCM                  with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
	fflush(stdout);

	/* Flash -> 可缓存的 RAM，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram[i] = p8_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 可缓存的 RAM，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram[i] = p16_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 可缓存的 RAM，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram[i] = p32_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 可缓存的 RAM，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_ram[i] = p64_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(cacheable area)   with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 不可缓存的 RAM，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram_nc[i] = p8_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 不可缓存的 RAM，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram_nc[i] = p16_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 不可缓存的 RAM，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram_nc[i] = p32_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 不可缓存的 RAM，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_ram_nc[i] = p64_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(uncacheable area) with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

#if BSP_CFG_DCACHE_ENABLED
	R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);

	/* DCache 进入 write-through 模式 */
	SCB_DisableDCache();
	MEMSYSCTL->MSCR |= MEMSYSCTL_MSCR_FORCEWT_Msk;
	__DSB();
	__ISB();
	reg = MEMSYSCTL->MSCR;
	SCB_EnableDCache();
	if ((reg & MEMSYSCTL_MSCR_FORCEWT_Msk) == 0) {
		puts("Set write-through failed");
		return;
	}
	puts("Turn on DCache, write-through mode");

	/* Flash -> DTCM，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_dtcm[i] = p8_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to DTCM                  with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> DTCM，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_dtcm[i] = p16_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to DTCM                  with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> DTCM，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_dtcm[i] = p32_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to DTCM                  with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> DTCM，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_dtcm[i] = p64_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to DTCM                  with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 可缓存的 RAM，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram[i] = p8_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 可缓存的 RAM，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram[i] = p16_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 可缓存的 RAM，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram[i] = p32_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 可缓存的 RAM，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_ram[i] = p64_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(cacheable area)   with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 不可缓存的 RAM，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram_nc[i] = p8_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 不可缓存的 RAM，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram_nc[i] = p16_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 不可缓存的 RAM，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram_nc[i] = p32_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* Flash -> 不可缓存的 RAM，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_ram_nc[i] = p64_flash[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from NorFlash to RAM(uncacheable area) with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
#endif
}

#pragma clang diagnostic pop

#endif
