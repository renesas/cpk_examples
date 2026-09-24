#include "test.h"

#if TEST_EN_HYPER_RAM

#include <stdio.h>
#include <stdlib.h>
#include "hal_data.h"
#include "hyper_ram.h"
#include "perf_counter/perf_counter.h"

#ifndef TEST_HYPER_RAM_EN_DMA
#define TEST_HYPER_RAM_EN_DMA	1
#endif

#define ENABLE_VOLATILE	1

#define CHECK_RESULT(r, mp, mf)	do { if (r) { if (mf != NULL) { puts(mf); } return r; } if (mp != NULL) { puts(mp); } } while(0)

#define CACHE_SIZE		(1024 * 64)

#if ENABLE_VOLATILE
#define VOLATILE	volatile
#else
#define VOLATILE
#endif

/* 位于 hal_data.c 中，FSP 生成 */
extern transfer_info_t g_dma1_info;

static VOLATILE uint8_t s_dma_done;
static VOLATILE uint8_t s_cache[CACHE_SIZE] __attribute__((section(".dtcm_noinit"))) __attribute__((aligned(4)));
static VOLATILE uint8_t s_hyper_ram[CACHE_SIZE] __attribute__((section(".ospi1_cs0_noinit")));
static VOLATILE uint8_t s_hyper_ram_nc[CACHE_SIZE] __attribute__((section(".ospi1_cs0_noinit_nocache")));
static VOLATILE uint8_t s_ram[CACHE_SIZE] __attribute__((section(".ram_noinit")));
static VOLATILE uint8_t s_ram_nc[CACHE_SIZE] __attribute__((section(".ram_noinit_nocache")));

static uint8_t checkFullChipNoCache(void);
static uint8_t checkFullChipWithSeq(void);
static void checkSpeedRead(void);
static void checkSpeedWrite(void);

uint32_t TestHyperRAM(uint32_t start_addr, uint32_t size)
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

	result = checkFullChipNoCache();
	CHECK_RESULT(result, "Full chip check PASS", "Full chip check FAILED");

	result = checkFullChipWithSeq();
	CHECK_RESULT(result, "Full chip check with ordered sequence PASS", "Full chip check with ordered sequence FAILED");

	R_BSP_SoftwareDelay(150, BSP_DELAY_UNITS_MILLISECONDS);
	checkSpeedWrite();
	R_BSP_SoftwareDelay(150, BSP_DELAY_UNITS_MILLISECONDS);
	checkSpeedRead();

	return 0;
}

void DMA1_Callback(transfer_callback_args_t *p_args)
{
	(void)p_args;

	s_dma_done = 1;
}

static uint8_t checkFullChipNoCache(void)
{
	uint8_t bit_num;
	uint8_t i8;
	uint32_t i, t;
	uint32_t val_r, expect;
	uint32_t mask;

	uint32_t mask_src = 0xA5A5A5A5;
	uint32_t *p32 = (uint32_t *)HYPER_RAM_MAP_START_ADDR;

	/* 先清理 SDRAM */
	memset(p32, 0xFF, HYPER_RAM_SIZE);
	__DSB();
	__ISB();

	for (i = 0; i < (HYPER_RAM_SIZE / 4); i++) {
		/* 计算 i 的有效位数 */
		bit_num = 0;
		t = i;
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
		p32[i] = ((mask & mask_src) | i);
	}

	for (i = 0; i < (HYPER_RAM_SIZE / 4); i++) {
		/* 计算 i 的有效位数 */
		bit_num = 0;
		t = i;
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
		expect = ((mask & mask_src) | i);
		val_r = p32[i];
		if (val_r != expect) {
			printf("%s => Failed at 0x%X\r\n", __FUNCTION__, HYPER_RAM_MAP_START_ADDR + i * 4);
			printf("Expect: 0x%X, Read: 0x%X\r\n", expect, val_r);
			printf("CMCFG1: 0x%X\r\n", R_XSPI1->CMCFGCS[0].CMCFG1);
			return 1;
		}
	}

	return 0;
}

static uint8_t checkFullChipWithSeq(void)
{
	uint32_t i, j;
	uint32_t repeat, remain;
	uint32_t val_r;

	uint8_t failed = 0;
	uint32_t *p32_cache = (uint32_t *)s_cache;
	uint32_t *p32_w = (uint32_t *)HYPER_RAM_MAP_START_ADDR;
	uint32_t *p32_r = (uint32_t *)HYPER_RAM_MAP_START_ADDR;

	/* 由于是先写入数据到 DTCM 的缓存，然后再写入 SDRAM，因此需要根据缓存的大小计算循环次数 */
	repeat = HYPER_RAM_SIZE / CACHE_SIZE;
	remain = HYPER_RAM_SIZE % CACHE_SIZE / 4;
	for (i = 0; i < repeat; i++) {
		for (j = 0; j < (CACHE_SIZE / 4); j++) {
			p32_cache[j] = i * CACHE_SIZE / 4 + j;
		}
		memcpy(p32_w, p32_cache, CACHE_SIZE);

		for (j = 0; j < (CACHE_SIZE / 4); j++) {
			val_r = *p32_r;
			if (val_r != p32_cache[j]) {
				printf("%s => Faild at 0x%X\r\n", __FUNCTION__, i * CACHE_SIZE + j * 4 + HYPER_RAM_MAP_START_ADDR);
				printf("%s => Write: 0x%X, Read: 0x%X\r\n", __FUNCTION__, p32_cache[j], val_r);
				failed = 1;
				break;
			}
			p32_r++;
		}

		if (failed) {
			break;
		}
		p32_w = p32_r;
	}

	if (failed == 0) {
		for (j = 0; j < remain; j++) {
			p32_cache[j] = i * CACHE_SIZE / 4 + j;
			*p32_w = p32_cache[j];
			p32_w++;
		}
		for (j = 0; j < remain; j++) {
			val_r = *p32_r;
			if (val_r != p32_cache[j]) {
				printf("%s => Faild at 0x%X\r\n", __FUNCTION__, i * CACHE_SIZE + j * 4 + HYPER_RAM_MAP_START_ADDR);

				failed = 1;
				break;
			}
			p32_r++;
		}
	}

	return failed;
}

static void checkSpeedRead(void)
{
	uint32_t i;
	float speed;
	VOLATILE int64_t time_start, time_end;
	VOLATILE int64_t tick_start, tick_end;
	VOLATILE uint32_t reg;

	VOLATILE uint8_t *p8_dtcm = s_cache;
	VOLATILE uint8_t *p8_ram = s_ram;
	VOLATILE uint8_t *p8_ram_nc = s_ram_nc;
	VOLATILE uint8_t *p8_hyper_ram = s_hyper_ram;
	VOLATILE uint8_t *p8_hyper_ram_nc = s_hyper_ram_nc;
	VOLATILE uint16_t *p16_dtcm = (VOLATILE uint16_t *)s_cache;
	VOLATILE uint16_t *p16_ram = (VOLATILE uint16_t *)s_ram;
	VOLATILE uint16_t *p16_ram_nc = (VOLATILE uint16_t *)s_ram_nc;
	VOLATILE uint16_t *p16_hyper_ram = (VOLATILE uint16_t *)s_hyper_ram;
	VOLATILE uint16_t *p16_hyper_ram_nc = (VOLATILE uint16_t *)s_hyper_ram_nc;
	VOLATILE uint32_t *p32_dtcm = (VOLATILE uint32_t *)s_cache;
	VOLATILE uint32_t *p32_ram = (VOLATILE uint32_t *)s_ram;
	VOLATILE uint32_t *p32_ram_nc = (VOLATILE uint32_t *)s_ram_nc;
	VOLATILE uint32_t *p32_hyper_ram = (VOLATILE uint32_t *)s_hyper_ram;
	VOLATILE uint32_t *p32_hyper_ram_nc = (VOLATILE uint32_t *)s_hyper_ram_nc;
	VOLATILE uint64_t *p64_ram = (VOLATILE uint64_t *)s_ram;
	VOLATILE uint64_t *p64_hyper_ram_nc = (VOLATILE uint64_t *)s_hyper_ram_nc;

	i = (uint32_t)s_hyper_ram;
	if ((i < HYPER_RAM_MAP_START_ADDR) || (i > HYPER_RAM_MAP_END_ADDR)) {
		printf("s_sdram[at 0x%X] not in SDRAM range\r\n", i);
		return;
	}
	i = (uint32_t)s_hyper_ram_nc;
	if ((i < HYPER_RAM_MAP_START_ADDR) || (i > HYPER_RAM_MAP_END_ADDR)) {
		printf("s_sdram_nc[at 0x%X] not in SDRAM range\r\n", i);
		return;
	}

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

	/* 可缓存的 HyperRAM 区域 -> DTCM，8bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_dtcm[i] = p8_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to DTCM                   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> DTCM，16bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_dtcm[i] = p16_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to DTCM                   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> DTCM，32bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_dtcm[i] = p32_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to DTCM                   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> DTCM，8bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_dtcm[i] = p8_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to DTCM                   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> DTCM，16bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_dtcm[i] = p16_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to DTCM                   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> DTCM，32bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_dtcm[i] = p32_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to DTCM                   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram[i] = p8_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram[i] = p16_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram[i] = p32_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram[i] = p8_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram[i] = p16_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

//	SCB_InvalidateDCache();
//	R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
//	memset(p32_ram, 0xA5, CACHE_SIZE);
//	memset(p32_hyper_ram_nc, 0xA5, CACHE_SIZE);
//	R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
//	SCB_InvalidateICache();
//	R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);

	/* 不可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram[i] = p32_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_ram[i] = p64_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (cacheable area)   with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 不可缓存的 RAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram_nc[i] = p8_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 不可缓存的 RAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram_nc[i] = p16_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 不可缓存的 RAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram_nc[i] = p32_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> 不可缓存的 RAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram_nc[i] = p8_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> 不可缓存的 RAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram_nc[i] = p16_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram_nc[i] = p32_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

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

	/* 可缓存的 HyperRAM 区域 -> DTCM，8bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_dtcm[i] = p8_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to DTCM                   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> DTCM，16bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_dtcm[i] = p16_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to DTCM                   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> DTCM，32bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_dtcm[i] = p32_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to DTCM                   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> DTCM，8bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_dtcm[i] = p8_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to DTCM                   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> DTCM，16bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_dtcm[i] = p16_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to DTCM                   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> DTCM，32bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_dtcm[i] = p32_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to DTCM                   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram[i] = p8_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram[i] = p16_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram[i] = p32_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram[i] = p8_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram[i] = p16_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram[i] = p32_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> 可缓存的 RAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_ram[i] = p64_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (cacheable area)   with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 不可缓存的 RAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram_nc[i] = p8_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 不可缓存的 RAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram_nc[i] = p16_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 HyperRAM 区域 -> 不可缓存的 RAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram_nc[i] = p32_hyper_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(cacheable area)   to RAM (uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> 不可缓存的 RAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_ram_nc[i] = p8_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 HyperRAM 区域 -> 不可缓存的 RAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_ram_nc[i] = p16_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram_nc[i] = p32_hyper_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Read from HyperRAM(uncacheable area) to RAM (uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
}

static void checkSpeedWrite(void)
{
	uint32_t i;
	float speed;
	VOLATILE int64_t time_start, time_end;
	VOLATILE int64_t tick_start, tick_end;
	VOLATILE uint32_t reg;

	VOLATILE uint8_t *p8_dtcm = s_cache;
	VOLATILE uint8_t *p8_ram = s_ram;
	VOLATILE uint8_t *p8_ram_nc = s_ram_nc;
	VOLATILE uint8_t *p8_hyper_ram = s_hyper_ram;
	VOLATILE uint8_t *p8_hyper_ram_nc = s_hyper_ram_nc;
	VOLATILE uint16_t *p16_dtcm = (VOLATILE uint16_t *)s_cache;
	VOLATILE uint16_t *p16_ram = (VOLATILE uint16_t *)s_ram;
	VOLATILE uint16_t *p16_ram_nc = (VOLATILE uint16_t *)s_ram_nc;
	VOLATILE uint16_t *p16_hyper_ram = (VOLATILE uint16_t *)s_hyper_ram;
	VOLATILE uint16_t *p16_hyper_ram_nc = (VOLATILE uint16_t *)s_hyper_ram_nc;
	VOLATILE uint32_t *p32_dtcm = (VOLATILE uint32_t *)s_cache;
	VOLATILE uint32_t *p32_ram = (VOLATILE uint32_t *)s_ram;
	VOLATILE uint32_t *p32_ram_nc = (VOLATILE uint32_t *)s_ram_nc;
	VOLATILE uint32_t *p32_hyper_ram = (VOLATILE uint32_t *)s_hyper_ram;
	VOLATILE uint32_t *p32_hyper_ram_nc = (VOLATILE uint32_t *)s_hyper_ram_nc;

	i = (uint32_t)s_hyper_ram;
	if ((i < HYPER_RAM_MAP_START_ADDR) || (i > HYPER_RAM_MAP_END_ADDR)) {
		printf("s_sdram[at 0x%X] not in SDRAM range\r\n", i);
		return;
	}
	i = (uint32_t)s_hyper_ram_nc;
	if ((i < HYPER_RAM_MAP_START_ADDR) || (i > HYPER_RAM_MAP_END_ADDR)) {
		printf("s_sdram_nc[at 0x%X] not in SDRAM range\r\n", i);
		return;
	}

	srand(71);
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_dtcm[i] = (uint32_t)rand();
	}
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram[i] = (uint32_t)rand();
	}
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram_nc[i] = (uint32_t)rand();
	}

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

	/* DTCM -> 可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram[i] = p8_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
	fflush(stdout);

	/* DTCM -> 可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram[i] = p16_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
	fflush(stdout);

	/* DTCM -> 可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram[i] = p32_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram_nc[i] = p8_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram_nc[i] = p16_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram_nc[i] = p32_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram[i] = p8_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram[i] = p16_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram[i] = p32_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram_nc[i] = p8_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram_nc[i] = p16_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram_nc[i] = p32_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram[i] = p8_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram[i] = p16_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram[i] = p32_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram_nc[i] = p8_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram_nc[i] = p16_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram_nc[i] = p32_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

#if TEST_HYPER_RAM_EN_DMA
	puts("DMA Operation");
	R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);

	/* DMA 写入：可缓存的 RAM -> 可缓存的 HyperRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	s_dma_done = 0;
	g_dma1_info.length = (uint16_t)(CACHE_SIZE / 8 - 1);
	g_dma1_info.p_dest = (void *)p8_hyper_ram;
	g_dma1_info.p_src = (void *)p8_ram;
	g_dma1_info.transfer_settings_word_b.size = TRANSFER_SIZE_8_BYTE;
	time_start = get_system_us();
	tick_start = get_system_ticks();
	R_DMAC_Open(g_dma1.p_ctrl, g_dma1.p_cfg);
	R_DMAC_Reconfigure(g_dma1.p_ctrl, &g_dma1_info);
	R_DMAC_Enable(g_dma1.p_ctrl);
	R_DMAC_SoftwareStart(g_dma1.p_ctrl, TRANSFER_START_MODE_REPEAT);
	while (s_dma_done == 0) {
		__NOP();
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("DMA   from RAM (cacheable area)   to HyperRAM(cacheable area)   with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
#endif

	/* 上位机使用 100ms 的时间戳，这个延时仅为了内容分在两个时间戳里 */
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

	/* DTCM -> 可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram[i] = p8_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram[i] = p16_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram[i] = p32_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram_nc[i] = p8_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram_nc[i] = p16_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram_nc[i] = p32_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to HyperRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram[i] = p8_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram[i] = p16_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram[i] = p32_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram_nc[i] = p8_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram_nc[i] = p16_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram_nc[i] = p32_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to HyperRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram[i] = p8_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram[i] = p16_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram[i] = p32_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_hyper_ram_nc[i] = p8_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_hyper_ram_nc[i] = p16_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 HyperRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_hyper_ram_nc[i] = p32_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to HyperRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
}

#endif
