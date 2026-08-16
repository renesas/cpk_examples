#include "hal_data.h"
#include "test.h"

#if TEST_EN_SDRAM

#include <stdio.h>
#include <stdlib.h>

#include "sdram.h"

#include "perf_counter/perf_counter.h"
#include "utils/log.h"
#include "utils/util.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

#ifndef TEST_SDRAM_EN_VOLATILE
#define TEST_SDRAM_EN_VOLATILE	1
#endif

#ifndef TEST_SDRAM_EN_AUTO_RF
#define TEST_SDRAM_EN_AUTO_RF	1
#endif

#ifndef TEST_SDRAM_EN_DMA
#define TEST_SDRAM_EN_DMA		0
#endif

#ifndef TEST_SDRAM_EN_8BIT
#define TEST_SDRAM_EN_8BIT		1
#endif

#ifndef TEST_SDRAM_EN_16BIT
#define TEST_SDRAM_EN_16BIT		1
#endif

#ifndef TEST_SDRAM_EN_32BIT
#define TEST_SDRAM_EN_32BIT		1
#endif

#ifndef TEST_SDRAM_EN_64BIT
#define TEST_SDRAM_EN_64BIT		1
#endif

#if TEST_SDRAM_EN_DMA
#define DMA_INSTANCE			g_dma0
#define DMA_CALLBACK			DMA0_Callback
#define DMA_FSP_GEN_INFO(inst)	UTIL_CONCAT(inst, _info)
#endif

#define CHECK_RESULT(r, mp, mf)	do { if (r) { if (mf != NULL) { puts(mf); } return 1; } if (mp != NULL) { puts(mp); } } while(0)

#define CACHE_SIZE		(1024 * 64)

#if TEST_SDRAM_EN_VOLATILE
#define VOLATILE	volatile
#else
#define VOLATILE
#endif

#if TEST_SDRAM_EN_DMA
/* 位于 hal_data.c 中，FSP 生成 */
extern transfer_info_t DMA_FSP_GEN_INFO(DMA_INSTANCE);
static VOLATILE uint8_t s_dma_done;
#endif

static VOLATILE uint8_t s_cache[CACHE_SIZE] __attribute__((section(".dtcm_noinit"))) __attribute__((aligned(4)));
static VOLATILE uint8_t s_sdram[CACHE_SIZE] __attribute__((section(".sdram_noinit")));
static VOLATILE uint8_t s_ram[CACHE_SIZE] __attribute__((section(".ram_noinit")));

#if BSP_CFG_DCACHE_ENABLED
static VOLATILE uint8_t s_sdram_nc[CACHE_SIZE] __attribute__((section(".sdram_noinit_nocache")));
static VOLATILE uint8_t s_ram_nc[CACHE_SIZE] __attribute__((section(".ram_noinit_nocache")));
#endif

static uint8_t checkAddressPin(void);
static uint8_t checkDataPin(uint32_t addr);
static uint8_t checkDQM(uint32_t addr);
static uint8_t checkFullChipNoCache(void);
static uint8_t checkFullChipWithSeq(void);
static uint8_t checkFullChipWithRand(uint32_t seed);
static void checkSpeedRead(void);
static void checkSpeedWrite(void);

#if TEST_SDRAM_EN_AUTO_RF
static uint8_t checkAutoRefresh(void);
#endif

uint32_t TestSDRAM(uint32_t start_addr, uint32_t size)
{
	uint8_t result;

	if ((start_addr < SDRAM_MAP_START_ADDR) || (start_addr > SDRAM_MAP_END_ADDR)) {
		puts("start_addr out of range");
		return 1;
	}
	if ((start_addr + size) > (SDRAM_MAP_START_ADDR + SDRAM_SIZE)) {
		size = SDRAM_MAP_START_ADDR + SDRAM_SIZE - start_addr;
	}

	if (SCB->CCR & SCB_CCR_DC_Msk) {
		SCB_DisableDCache();
		puts("Turn off DCache");
	}
	else {
		puts("DCache already disable");
	}

	result = checkDataPin(start_addr);
	CHECK_RESULT(result, "Data pin check PASS", "Data pin check FAILED");

	result = checkAddressPin();
	CHECK_RESULT(result, "Address pin check PASS", "Address pin check FAILED");

	result = checkDQM(start_addr);
	CHECK_RESULT(result, "DQM pin check PASS", "DQM pin check FAILED");

	result = checkFullChipNoCache();
	CHECK_RESULT(result, "Full chip check PASS", "Full chip check FAILED");

	result = checkFullChipWithSeq();
	CHECK_RESULT(result, "Full chip check with ordered sequence PASS", "Full chip check with ordered sequence FAILED");

	result = checkFullChipWithRand(42);
	CHECK_RESULT(result, "Full chip check with random sequence PASS", "Full chip check with random sequence FAILED");

	result = checkAutoRefresh();
	CHECK_RESULT(result, "Auto refresh check PASS", "Auto refresh check FAILED");

#if TEST_SDRAM_EN_DMA
	s_dma_done = 0;
#endif

	checkSpeedWrite();
	/* 上位机使用 100ms 的时间戳，这个延时仅为了内容分在两个时间戳里 */
	R_BSP_SoftwareDelay(300, BSP_DELAY_UNITS_MILLISECONDS);
	checkSpeedRead();

	return 0;
}

#if TEST_SDRAM_EN_DMA
void DMA_CALLBACK(transfer_callback_args_t *p_args)
{
	(void)p_args;

	s_dma_done = 1;
}
#endif

static uint8_t checkAddressPin(void)
{
	uint32_t i, j;
	uint32_t addr, addr_vali, val_r;
	uint32_t interval_addr1, interval_addr2;
	uint32_t interval_mask;

	/* 以 32bit 宽度写入，A1、A0 就检查不到，因此需要减去 2 */
	uint32_t avaliable_addr_width = SDRAM_ADDR_WIDTH - 2;
	uint32_t pattern = 0xA5A5A5A5;
	uint32_t anti_pattern = 0x5A5A5A5A;
	uint32_t *p32_cache = (uint32_t *)s_cache;
	uint32_t *p32_base = (uint32_t *)SDRAM_MAP_START_ADDR;
	uint32_t *p32_w = (uint32_t *)SDRAM_MAP_START_ADDR;
	uint32_t *p32_cur = NULL;
	uint32_t *p32_validate = NULL;

	/* SDRAM 的起始地址需要先写入。起始地址所有地址线都会置 0 */
	*p32_base = anti_pattern;
	/* 每条地址线单独置 1，根据地址宽度计算所有需要验证的地址 */
	for (i = 2; i < SDRAM_ADDR_WIDTH; i++) {
		p32_cache[i - 2] = 0x01UL << i;
	}

	/* 给所有需要验证的地址写入预设值 */
	for (i = 0; i < avaliable_addr_width; i++) {
		p32_w = (uint32_t *)(SDRAM_MAP_START_ADDR + p32_cache[i]);
		*p32_w = anti_pattern;
	}
	__DSB();
	__ISB();

	/* 再次遍历需要验证的地址，给第 i 个地址写入另一个预设值，检查其它地址的值有没有被修改 */
	for (i = 0; i < avaliable_addr_width; i++) {
		/* 给第 i 个地址写入另一个预设值 */
		addr = SDRAM_MAP_START_ADDR + p32_cache[i];
		p32_cur = (uint32_t *)addr;
		*p32_cur = pattern;
		__DSB();
		__ISB();

		/* 第 i 个地址读回，看看刚刚写入的值对不对 */
		val_r = *p32_cur;
		if (val_r != pattern) {
			printf("%s => Failed at 0x%X\r\n", __FUNCTION__, addr);
			printf("%s => Write: 0x%X\r\nRead:  0x%X\r\n", __FUNCTION__, pattern, val_r);
			return 1;
		}

		/* 检查起始地址的值有没有被修改 */
		val_r = *p32_base;
		if (val_r != anti_pattern) {
			printf("%s => Address 0x%X affects base address 0x%X\r\n", __FUNCTION__, addr, SDRAM_MAP_START_ADDR);
			printf("%s => Expect base val: 0x%X, read 0x%X\r\n", __FUNCTION__, anti_pattern, val_r);
			return 1;
		}

		/* 检查其它地址的值有没有被修改 */
		for (j = 0; j < avaliable_addr_width; j++) {
			if (j == i) {
				continue;
			}

			addr_vali = SDRAM_MAP_START_ADDR + p32_cache[j];
			p32_validate = (uint32_t *)addr_vali;
			val_r = *p32_validate;
			if (val_r != anti_pattern) {
				printf("%s => Address 0x%X effects 0x%X\r\n", __FUNCTION__, addr, addr_vali);
				printf("%s => Expect: 0x%X, read: 0x%X\r\n", __FUNCTION__, anti_pattern, val_r);
				return 1;
			}
		}

		/* 把第 i 个地址的值改回去 */
		*p32_cur = anti_pattern;
	}

	/* 根据地址宽度计算间隔地址。间隔地址：两个地址按位取反，这样在访问这两个地址时，就需要将所有地址线翻转 */
	interval_addr1 = 0x01;
	for (i = 1; i < (SDRAM_ADDR_WIDTH / 2); i++) {
		interval_addr1 |= (0x01 << (i * 2));
	}
	/* 根据地址宽度计算第二个间隔地址的掩码，要根据掩码计算第二个间隔地址 */
	interval_mask = 0xFFFFFFFF;
	for (i = 0; i < SDRAM_ADDR_WIDTH; i++) {
		interval_mask <<= 0x01;
	}
	interval_addr2 = ~interval_addr1;
	interval_addr2 = interval_addr2 & (~interval_mask);
	interval_addr1 += SDRAM_MAP_START_ADDR;
	interval_addr2 += SDRAM_MAP_START_ADDR;
	/* 取 4K 字节大小做测试 */
	interval_addr1 &= 0xFFFFF000;
	interval_addr2 &= 0xFFFFF000;
	/* 输出的间隔地址应该类似 0x68555000 和 0x68AAA000 */
	printf("interval_addr1 = 0x%X\r\n", interval_addr1);
	printf("interval_addr2 = 0x%X\r\n", interval_addr2);

	/* 多次循环对这两个地址读写值，检查可靠性。 */
	p32_w = (uint32_t *)interval_addr1;
	p32_base = (uint32_t *)interval_addr2;
	for (j = 0; j < 1000; j++) {
		/* 4K 字节对应 1024 个字 */
		for (i = 0; i < 1024; i++) {
			/* 每 16 字节，即 4 个字切换写入的数据 */
			if ((i % 4) == 0) {
				p32_w[i] = pattern;
				p32_base[i] = anti_pattern;
			}
			else {
				p32_w[i] = anti_pattern;
				p32_base[i] = pattern;
			}
		}
		__DSB();
		__ISB();
		for (i = 0; i < 1024; i++) {
			if ((i % 4) == 0) {
				val_r = p32_w[i];
				if (val_r != pattern) {
					printf("%s => Failed at 0x%X\r\n", __FUNCTION__,  interval_addr1 + i * 4);
					printf("Expect: 0x%X, Read: 0x%X\r\n", pattern, val_r);
					return 1;
				}

				val_r = p32_base[i];
				if (val_r != anti_pattern) {
					printf("%s => Failed at 0x%X\r\n", __FUNCTION__,  interval_addr2 + i * 4);
					printf("Expect: 0x%X, Read: 0x%X\r\n", anti_pattern, val_r);
					return 1;
				}
			}
			else {
				val_r = p32_w[i];
				if (val_r != anti_pattern) {
					printf("%s => Failed at 0x%X\r\n", __FUNCTION__,  interval_addr1 + i * 4);
					printf("Expect: 0x%X, Read: 0x%X\r\n", pattern, val_r);
					return 1;
				}

				val_r = p32_base[i];
				if (val_r != pattern) {
					printf("%s => Failed at 0x%X\r\n", __FUNCTION__,  interval_addr2 + i * 4);
					printf("Expect: 0x%X, Read: 0x%X\r\n", pattern, val_r);
					return 1;
				}
			}
		}
		__DSB();
		__ISB();
	}

	return 0;
}

static uint8_t checkDataPin(uint32_t addr)
{
	uint32_t i;
	volatile uint32_t val_r;

	volatile uint32_t val_w = 0x01;
	volatile uint32_t *p32 = (uint32_t *)addr;

	/* 将每条数据线的单独置 1 进行读写，验证数据线的连接 */
	for (i = 0; i < SDRAM_DATA_WIDTH; i++) {
		*p32 = val_w;
		__DSB();
		__ISB();
		val_r = *p32;
		if (val_r != val_w) {
			printf("%s => Failed, val_w: 0x%X, val_r: 0x%X\r\n", __FUNCTION__, val_w, val_r);
			return 1;
		}
		val_w <<= 1;
	}

	return 0;
}

static uint8_t checkDQM(uint32_t addr)
{
	uint8_t i, j;
	uint8_t val_r;

	uint8_t val_w = 0xA5;
	uint8_t val_w_anti = 0x5A;
	uint8_t *p8_w = (uint8_t *)addr;
	uint8_t *p8_r = (uint8_t *)addr;

	/* 写清空写入地址及后面的部分空间 */
	memset(p8_w, 0, 16);
	__DSB();
	__ISB();

	/* 向目标地址写入预设值
	 * TODO 这四个字节分别写入不同的值也许更好 */
	for (i = 0; i < 4; i++) {
		p8_w[i] = val_w;
	}
	__DSB();
	__ISB();

	for (i = 0; i < 4; i++) {
		val_r = p8_r[i];
		if (val_r != val_w) {
			printf("%s => Failed at 0x%X\r\n", __FUNCTION__, addr + i);
			printf("%s => Write: 0x%X, Read: 0x%X\r\n", __FUNCTION__, val_w, val_r);
			return 1;
		}
		__DSB();
		__ISB();

		p8_w[i] = val_w_anti;
		val_r = p8_r[i];
		if (val_r != val_w_anti) {
			printf("%s => Failed at 0x%X\r\n", __FUNCTION__, addr + i);
			printf("%s => Write: 0x%X, Read: 0x%X\r\n", __FUNCTION__, val_w_anti, val_r);
			return 1;
		}
		__DSB();
		__ISB();

		for (j = 0; j < 4; j++) {
			if (j == i) {
				continue;
			}

			val_r = p8_r[j];
			if (val_r != val_w) {
				printf("%s => Failed at 0x%X\r\n", __FUNCTION__, addr + j);
				printf("%s => Write: 0x%X, Read: 0x%X\r\n", __FUNCTION__, val_w_anti, val_r);
				return 1;
			}
		}

		p8_w[i] = val_w;
	}

	return 0;
}

static uint8_t checkFullChipNoCache(void)
{
	uint8_t bit_num;
	uint8_t i8;
	uint32_t i, t;
	uint32_t val_r, expect;
	uint32_t mask;

	uint32_t mask_src = 0xA5A5A5A5;
	uint32_t *p32 = (uint32_t *)SDRAM_MAP_START_ADDR;

	/* 先清理 SDRAM */
	memset(p32, 0xFF, SDRAM_SIZE);
	__DSB();
	__ISB();

	for (i = 0; i < (SDRAM_SIZE / 4); i++) {
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

	for (i = 0; i < (SDRAM_SIZE / 4); i++) {
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
			printf("%s => Failed at 0x%X\r\n", __FUNCTION__, SDRAM_MAP_START_ADDR + i * 4);
			printf("Expect: 0x%X, Read: 0x%X\r\n", expect, val_r);
			return 1;
		}
	}

	return 0;
}

/**
 * @brief	向全片写入有序序列再读回
 * @return	0 = PASS；1 = FAILED
 */
static uint8_t checkFullChipWithSeq(void)
{
	uint32_t i, j;
	uint32_t repeat, remain;
	uint32_t val_r;

	uint8_t failed = 0;
	uint32_t *p32_cache = (uint32_t *)s_cache;
	uint32_t *p32_w = (uint32_t *)SDRAM_MAP_START_ADDR;
	uint32_t *p32_r = (uint32_t *)SDRAM_MAP_START_ADDR;

	/* 由于是先写入数据到 DTCM 的缓存，然后再写入 SDRAM，因此需要根据缓存的大小计算循环次数 */
	repeat = SDRAM_SIZE / CACHE_SIZE;
	remain = SDRAM_SIZE % CACHE_SIZE / 4;
	for (i = 0; i < repeat; i++) {
		for (j = 0; j < (CACHE_SIZE / 4); j++) {
			p32_cache[j] = i * CACHE_SIZE / 4 + j;
		}
		memcpy(p32_w, p32_cache, CACHE_SIZE);

		for (j = 0; j < (CACHE_SIZE / 4); j++) {
			val_r = *p32_r;
			if (val_r != p32_cache[j]) {
				printf("%s => Faild at 0x%X\r\n", __FUNCTION__, i * CACHE_SIZE + j * 4 + SDRAM_MAP_START_ADDR);
				printf("%s => Write: 0x%X, Read: 0x%X\r\n", __FUNCTION__, p32_cache[j], val_r);

				if (checkDataPin(i * CACHE_SIZE + j * 4 + SDRAM_MAP_START_ADDR) == 0) {
					printf("%s => checkDataPin at this address PASS\r\n", __FUNCTION__);
				}

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
				printf("%s => Faild at 0x%X\r\n", __FUNCTION__, i * CACHE_SIZE + j * 4 + SDRAM_MAP_START_ADDR);

				failed = 1;
				break;
			}
			p32_r++;
		}
	}

	return failed;
}

static uint8_t checkFullChipWithRand(uint32_t seed)
{
	uint32_t i, j;
	uint32_t repeat, remain;
	uint32_t val_r;

	uint32_t *p32_cache = (uint32_t *)s_cache;
	uint32_t *p32_w = (uint32_t *)SDRAM_MAP_START_ADDR;
	uint32_t *p32_r = (uint32_t *)SDRAM_MAP_START_ADDR;

	repeat = SDRAM_SIZE / CACHE_SIZE;
	remain = SDRAM_SIZE % CACHE_SIZE / 4;
	srand(seed);
	for (i = 0; i < repeat; i++) {
		for (j = 0; j < (CACHE_SIZE / 4); j++) {
			p32_cache[j] = (uint32_t)rand();
			*p32_w = p32_cache[j];
			p32_w++;
		}

		for (j = 0; j < (CACHE_SIZE / 4); j++) {
			val_r = *p32_r;
			if (val_r != p32_cache[j]) {
				printf("%s => Faild at 0x%X\r\n\r\n", __FUNCTION__, i * CACHE_SIZE + j * 4 + SDRAM_MAP_START_ADDR);
				return 1;
			}
			p32_r++;
		}
	}

	for (j = 0; j < remain; j++) {
		p32_cache[j] = (uint32_t)rand();
		*p32_w = p32_cache[j];
		p32_w++;
	}
	for (j = 0; j < remain; j++) {
		val_r = *p32_r;
		if (val_r != p32_cache[j]) {
			printf("%s => Faild at 0x%X\r\n\r\n", __FUNCTION__, i * CACHE_SIZE + j * 4 + SDRAM_MAP_START_ADDR);
			return 1;
		}
		p32_r++;
	}

	return 0;
}

static void checkSpeedRead(void)
{
    uint32_t i;
    float speed;
    VOLATILE int64_t time_start, time_end;
    VOLATILE int64_t tick_start, tick_end;

    VOLATILE uint8_t *p8_dtcm = s_cache;
    VOLATILE uint8_t *p8_ram = s_ram;
    VOLATILE uint8_t *p8_sdram = s_sdram;
    VOLATILE uint16_t *p16_dtcm = (VOLATILE uint16_t *)s_cache;
    VOLATILE uint16_t *p16_ram = (VOLATILE uint16_t *)s_ram;
    VOLATILE uint16_t *p16_sdram = (VOLATILE uint16_t *)s_sdram;
    VOLATILE uint32_t *p32_dtcm = (VOLATILE uint32_t *)s_cache;
    VOLATILE uint32_t *p32_ram = (VOLATILE uint32_t *)s_ram;
    VOLATILE uint32_t *p32_sdram = (VOLATILE uint32_t *)s_sdram;
    VOLATILE uint64_t *p64_dtcm = (VOLATILE uint64_t *)s_cache;
	VOLATILE uint64_t *p64_ram = (VOLATILE uint64_t *)s_ram;
	VOLATILE uint64_t *p64_sdram = (VOLATILE uint64_t *)s_sdram;

#if BSP_CFG_DCACHE_ENABLED
	volatile uint32_t reg;
	VOLATILE uint8_t *p8_ram_nc = s_ram_nc;
	VOLATILE uint8_t *p8_sdram_nc = s_sdram_nc;
	VOLATILE uint16_t *p16_ram_nc = (VOLATILE uint16_t *)s_ram_nc;
	VOLATILE uint16_t *p16_sdram_nc = (VOLATILE uint16_t *)s_sdram_nc;
	VOLATILE uint32_t *p32_ram_nc = (VOLATILE uint32_t *)s_ram_nc;
	VOLATILE uint32_t *p32_sdram_nc = (VOLATILE uint32_t *)s_sdram_nc;
	VOLATILE uint64_t *p64_ram_nc = (VOLATILE uint64_t *)s_ram_nc;
	VOLATILE uint64_t *p64_sdram_nc = (VOLATILE uint64_t *)s_sdram_nc;
#endif

    i = (uint32_t)s_sdram;
    if ((i < SDRAM_MAP_START_ADDR) || (i > SDRAM_MAP_END_ADDR)) {
        printf("s_sdram[at 0x%X] not in SDRAM range\r\n", i);
        return;
    }

#if BSP_CFG_DCACHE_ENABLED
    i = (uint32_t)s_sdram_nc;
    if ((i < SDRAM_MAP_START_ADDR) || (i > SDRAM_MAP_END_ADDR)) {
        printf("s_sdram_nc[at 0x%X] not in SDRAM range\r\n", i);
        return;
    }
#endif

#if BSP_CFG_DCACHE_ENABLED
    /* 关闭 forced write-through，进入 write-back 模式 */
    SCB_DisableDCache();
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

    /* 可缓存的 SDRAM 区域 -> DTCM，8bit 宽度 */
    SCB_InvalidateDCache();
    tick_start = get_system_ticks();
    time_start = get_system_us();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_dtcm[i] = p8_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
        printf("Read from SDRAM(cacheable area)   to DTCM                   with  8bit width, ");
	#else
        printf("Read from SDRAM to DTCM with  8bit width, ");
	#endif
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> DTCM，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_dtcm[i] = p16_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
        printf("Read from SDRAM(cacheable area)   to DTCM                   with 16bit width, ");
	#else
        printf("Read from SDRAM to DTCM with 16bit width, ");
	#endif
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> DTCM，32bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 4); i++) {
        p32_dtcm[i] = p32_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
        printf("Read from SDRAM(cacheable area)   to DTCM                   with 32bit width, ");
	#else
        printf("Read from SDRAM to DTCM with 32bit width, ");
	#endif
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> DTCM，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_dtcm[i] = p64_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
        printf("Read from SDRAM(cacheable area)   to DTCM                   with 64bit width, ");
	#else
        printf("Read from SDRAM to DTCM with 64bit width, ");
	#endif
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

#if BSP_CFG_DCACHE_ENABLED
    /* 不可缓存的 SDRAM 区域 -> DTCM，8bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_dtcm[i] = p8_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to DTCM                   with  8bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> DTCM，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_dtcm[i] = p16_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to DTCM                   with 16bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> DTCM，32bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 4); i++) {
        p32_dtcm[i] = p32_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to DTCM                   with 32bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> DTCM，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_dtcm[i] = p64_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to DTCM                   with 64bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }
#endif

    /* 可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，8bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_ram[i] = p8_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
        printf("Read from SDRAM(cacheable area)   to RAM (cacheable area)   with  8bit width, ");
	#else
        printf("Read from SDRAM to RAM  with  8bit width, ");
	#endif
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_ram[i] = p16_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
        printf("Read from SDRAM(cacheable area)   to RAM (cacheable area)   with 16bit width, ");
	#else
        printf("Read from SDRAM to RAM  with 16bit width, ");
	#endif
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，32bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 4); i++) {
        p32_ram[i] = p32_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
        printf("Read from SDRAM(cacheable area)   to RAM (cacheable area)   with 32bit width, ");
	#else
        printf("Read from SDRAM to RAM  with 32bit width, ");
	#endif
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_ram[i] = p64_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
        printf("Read from SDRAM(cacheable area)   to RAM (cacheable area)   with 64bit width, ");
	#else
        printf("Read from SDRAM to RAM  with 64bit width, ");
	#endif
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

#if BSP_CFG_DCACHE_ENABLED
    /* 可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，8bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_ram_nc[i] = p8_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (uncacheable area) with  8bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_ram_nc[i] = p16_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (uncacheable area) with 16bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，32bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 4); i++) {
        p32_ram_nc[i] = p32_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (uncacheable area) with 32bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_ram_nc[i] = p64_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (uncacheable area) with 64bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，8bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_ram[i] = p8_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (cacheable area)   with  8bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_ram[i] = p16_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (cacheable area)   with 16bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，32bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 4); i++) {
        p32_ram[i] = p32_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (cacheable area)   with 32bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_ram[i] = p64_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (cacheable area)   with 64bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，8bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_ram_nc[i] = p8_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (uncacheable area) with  8bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_ram_nc[i] = p16_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (uncacheable area) with 16bit width, ");
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
        p32_ram_nc[i] = p32_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (uncacheable area) with 32bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_ram_nc[i] = p64_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (uncacheable area) with 64bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 上位机使用 100ms 的时间戳，这个延时仅为了内容分在两个时间戳里 */
    R_BSP_SoftwareDelay(300, BSP_DELAY_UNITS_MILLISECONDS);

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

    /* 可缓存的 SDRAM 区域 -> DTCM，8bit 宽度 */
    SCB_InvalidateDCache();
    tick_start = get_system_ticks();
    time_start = get_system_us();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_dtcm[i] = p8_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to DTCM                   with  8bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> DTCM，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_dtcm[i] = p16_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to DTCM                   with 16bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> DTCM，32bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 4); i++) {
        p32_dtcm[i] = p32_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to DTCM                   with 32bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> DTCM，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_dtcm[i] = p64_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to DTCM                   with 64bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> DTCM，8bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_dtcm[i] = p8_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to DTCM                   with  8bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> DTCM，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_dtcm[i] = p16_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to DTCM                   with 16bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> DTCM，32bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 4); i++) {
        p32_dtcm[i] = p32_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to DTCM                   with 32bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> DTCM，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_dtcm[i] = p64_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to DTCM                   with 64bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，8bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_ram[i] = p8_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (cacheable area)   with  8bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_ram[i] = p16_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (cacheable area)   with 16bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，32bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 4); i++) {
        p32_ram[i] = p32_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (cacheable area)   with 32bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_ram[i] = p64_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (cacheable area)   with 64bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，8bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_ram_nc[i] = p8_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (uncacheable area) with  8bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_ram_nc[i] = p16_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (uncacheable area) with 16bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，32bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 4); i++) {
        p32_ram_nc[i] = p32_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (uncacheable area) with 32bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_ram_nc[i] = p64_sdram[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(cacheable area)   to RAM (uncacheable area) with 64bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，8bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_ram[i] = p8_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (cacheable area)   with  8bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_ram[i] = p16_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (cacheable area)   with 16bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，32bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 4); i++) {
        p32_ram[i] = p32_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (cacheable area)   with 32bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 可缓存的 RAM 区域，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_ram[i] = p64_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (cacheable area)   with 64bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，8bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < CACHE_SIZE; i++) {
        p8_ram_nc[i] = p8_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (uncacheable area) with  8bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，16bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 2); i++) {
        p16_ram_nc[i] = p16_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (uncacheable area) with 16bit width, ");
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
        p32_ram_nc[i] = p32_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (uncacheable area) with 32bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }

    /* 不可缓存的 SDRAM 区域 -> 不可缓存的 RAM 区域，64bit 宽度 */
    SCB_InvalidateDCache();
    time_start = get_system_us();
    tick_start = get_system_ticks();
    for (i = 0; i < (CACHE_SIZE / 8); i++) {
        p64_ram_nc[i] = p64_sdram_nc[i];
    }
    tick_end = get_system_ticks();
    time_end = get_system_us();
    if (time_start != time_end) {
        speed = (float)CACHE_SIZE / (float)(time_end - time_start);
        speed = speed * 1000000 / 1024 / 1024;
        printf("Read from SDRAM(uncacheable area) to RAM (uncacheable area) with 64bit width, ");
        printf("using cycle: %llu, ", tick_end - tick_start);
        printf("using time: %llu us, ", time_end - time_start);
        printf("speed: %.2f MB/s\r\n", speed);
    }
    else {
        puts("Test size is too small with this case");
    }
#endif /* #if BSP_CFG_DCACHE_ENABLED */
}

static void checkSpeedWrite(void)
{
	uint32_t i;
	float speed;
	VOLATILE int64_t time_start, time_end;
	VOLATILE int64_t tick_start, tick_end;

	VOLATILE uint8_t *p8_dtcm = s_cache;
	VOLATILE uint8_t *p8_ram = s_ram;
	VOLATILE uint8_t *p8_sdram = s_sdram;
	VOLATILE uint16_t *p16_dtcm = (VOLATILE uint16_t *)s_cache;
	VOLATILE uint16_t *p16_ram = (VOLATILE uint16_t *)s_ram;
	VOLATILE uint16_t *p16_sdram = (VOLATILE uint16_t *)s_sdram;
	VOLATILE uint32_t *p32_dtcm = (VOLATILE uint32_t *)s_cache;
	VOLATILE uint32_t *p32_ram = (VOLATILE uint32_t *)s_ram;
	VOLATILE uint32_t *p32_sdram = (VOLATILE uint32_t *)s_sdram;
	VOLATILE uint64_t *p64_dtcm = (VOLATILE uint64_t *)s_cache;
	VOLATILE uint64_t *p64_ram = (VOLATILE uint64_t *)s_ram;
	VOLATILE uint64_t *p64_sdram = (VOLATILE uint64_t *)s_sdram;

#if BSP_CFG_DCACHE_ENABLED
	volatile uint32_t reg;
	VOLATILE uint8_t *p8_ram_nc = s_ram_nc;
	VOLATILE uint8_t *p8_sdram_nc = s_sdram_nc;
	VOLATILE uint16_t *p16_ram_nc = (VOLATILE uint16_t *)s_ram_nc;
	VOLATILE uint16_t *p16_sdram_nc = (VOLATILE uint16_t *)s_sdram_nc;
	VOLATILE uint32_t *p32_ram_nc = (VOLATILE uint32_t *)s_ram_nc;
	VOLATILE uint32_t *p32_sdram_nc = (VOLATILE uint32_t *)s_sdram_nc;
	VOLATILE uint64_t *p64_ram_nc = (VOLATILE uint64_t *)s_ram_nc;
	VOLATILE uint64_t *p64_sdram_nc = (VOLATILE uint64_t *)s_sdram_nc;
#endif

	i = (uint32_t)s_sdram;
	if ((i < SDRAM_MAP_START_ADDR) || (i > SDRAM_MAP_END_ADDR)) {
		printf("s_sdram[at 0x%X] not in SDRAM range\r\n", i);
		return;
	}
#if BSP_CFG_DCACHE_ENABLED
	i = (uint32_t)s_sdram_nc;
	if ((i < SDRAM_MAP_START_ADDR) || (i > SDRAM_MAP_END_ADDR)) {
		printf("s_sdram_nc[at 0x%X] not in SDRAM range\r\n", i);
		return;
	}
#endif

	srand(71);
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_dtcm[i] = (uint32_t)rand();
	}
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_ram[i] = (uint32_t)rand();
	}
#if BSP_CFG_DCACHE_ENABLED
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
#endif

	/* DTCM -> 可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram[i] = p8_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
		printf("Write from DTCM                   to SDRAM(cacheable area)   with  8bit width, ");
	#else
		printf("Write from DTCM to SDRAM with  8bit width, ");
	#endif
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram[i] = p16_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
		printf("Write from DTCM                   to SDRAM(cacheable area)   with 16bit width, ");
	#else
		printf("Write from DTCM to SDRAM with 16bit width, ");
	#endif
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram[i] = p32_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
		printf("Write from DTCM                   to SDRAM(cacheable area)   with 32bit width, ");
	#else
		printf("Write from DTCM to SDRAM with 32bit width, ");
	#endif
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram[i] = p64_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
		printf("Write from DTCM                   to SDRAM(cacheable area)   with 64bit width, ");
	#else
		printf("Write from DTCM to SDRAM with 64bit width, ");
	#endif
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

#if BSP_CFG_DCACHE_ENABLED
	/* DTCM -> 不可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram_nc[i] = p8_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram_nc[i] = p16_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram_nc[i] = p32_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram_nc[i] = p64_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(uncacheable area) with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
#endif

	/* 可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram[i] = p8_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
		printf("Write from RAM (cacheable area)   to SDRAM(cacheable area)   with  8bit width, ");
	#else
		printf("Write from RAM  to SDRAM with  8bit width, ");
	#endif
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram[i] = p16_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
		printf("Write from RAM (cacheable area)   to SDRAM(cacheable area)   with 16bit width, ");
	#else
		printf("Write from RAM  to SDRAM with 16bit width, ");
	#endif
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram[i] = p32_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
		printf("Write from RAM (cacheable area)   to SDRAM(cacheable area)   with 32bit width, ");
	#else
		printf("Write from RAM  to SDRAM with 32bit width, ");
	#endif
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram[i] = p64_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
	#if BSP_CFG_DCACHE_ENABLED
		printf("Write from RAM (cacheable area)   to SDRAM(cacheable area)   with 64bit width, ");
	#else
		printf("Write from RAM  to SDRAM with 64bit width, ");
	#endif
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

#if BSP_CFG_DCACHE_ENABLED
	/* 可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram_nc[i] = p8_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram_nc[i] = p16_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram_nc[i] = p32_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram_nc[i] = p64_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(uncacheable area) with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram[i] = p8_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram[i] = p16_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram[i] = p32_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram[i] = p64_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(cacheable area)   with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram_nc[i] = p8_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram_nc[i] = p16_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram_nc[i] = p32_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram_nc[i] = p64_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(uncacheable area) with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
#endif

#if TEST_SDRAM_EN_DMA
	memset((void *)SDRAM_MAP_START_ADDR, 0, SDRAM_SIZE);
	/* 上位机使用 100ms 的时间戳，这个延时仅为了内容分在两个时间戳里 */
	R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);

	puts("DMA Operation");

	/* DMA 写入：不可缓存的 RAM -> 可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	s_dma_done = 0;
	DMA_FSP_GEN_INFO(DMA_INSTANCE).length = (uint16_t)(CACHE_SIZE - 1);
	DMA_FSP_GEN_INFO(DMA_INSTANCE).p_dest = (void *)p8_sdram;
	DMA_FSP_GEN_INFO(DMA_INSTANCE).p_src = (void *)p8_ram;
	DMA_FSP_GEN_INFO(DMA_INSTANCE).transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
	time_start = get_system_us();
	tick_start = get_system_ticks();
	R_DMAC_Open(DMA_INSTANCE.p_ctrl, DMA_INSTANCE.p_cfg);
	R_DMAC_Reconfigure(DMA_INSTANCE.p_ctrl, &DMA_FSP_GEN_INFO(DMA_INSTANCE));
	R_DMAC_Enable(DMA_INSTANCE.p_ctrl);
	R_DMAC_SoftwareStart(DMA_INSTANCE.p_ctrl, TRANSFER_START_MODE_REPEAT);
	while (s_dma_done == 0) {
		__NOP();
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("DMA   from RAM (uncacheable area) to SDRAM(uncacheable area) with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
#endif

	/* 上位机使用 100ms 的时间戳，这个延时仅为了内容分在两个时间戳里 */
	R_BSP_SoftwareDelay(150, BSP_DELAY_UNITS_MILLISECONDS);
	memset((void *)SDRAM_MAP_START_ADDR, 0, SDRAM_SIZE);
	R_BSP_SoftwareDelay(150, BSP_DELAY_UNITS_MILLISECONDS);

#if BSP_CFG_DCACHE_ENABLED
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

	/* DTCM -> 可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	tick_start = get_system_ticks();
	time_start = get_system_us();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram[i] = p8_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram[i] = p16_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram[i] = p32_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram[i] = p64_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(cacheable area)   with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram_nc[i] = p8_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram_nc[i] = p16_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram_nc[i] = p32_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* DTCM -> 不可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram_nc[i] = p64_dtcm[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from DTCM                   to SDRAM(uncacheable area) with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram[i] = p8_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram[i] = p16_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram[i] = p32_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram[i] = p64_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(cacheable area)   with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram_nc[i] = p8_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram_nc[i] = p16_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram_nc[i] = p32_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram_nc[i] = p64_ram[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (cacheable area)   to SDRAM(uncacheable area) with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram[i] = p8_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(cacheable area)   with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram[i] = p16_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(cacheable area)   with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram[i] = p32_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(cacheable area)   with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram[i] = p64_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(cacheable area)   with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，8bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < CACHE_SIZE; i++) {
		p8_sdram_nc[i] = p8_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(uncacheable area) with  8bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，16bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 2); i++) {
		p16_sdram_nc[i] = p16_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(uncacheable area) with 16bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，32bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_sdram_nc[i] = p32_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(uncacheable area) with 32bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}

	/* 不可缓存的 RAM 区域 -> 不可缓存的 SDRAM 区域，64bit 宽度 */
	SCB_InvalidateDCache();
	time_start = get_system_us();
	tick_start = get_system_ticks();
	for (i = 0; i < (CACHE_SIZE / 8); i++) {
		p64_sdram_nc[i] = p64_ram_nc[i];
	}
	tick_end = get_system_ticks();
	time_end = get_system_us();
	if (time_start != time_end) {
		speed = (float)CACHE_SIZE / (float)(time_end - time_start);
		speed = speed * 1000000 / 1024 / 1024;
		printf("Write from RAM (uncacheable area) to SDRAM(uncacheable area) with 64bit width, ");
		printf("using cycle: %llu, ", tick_end - tick_start);
		printf("using time: %llu us, ", time_end - time_start);
		printf("speed: %.2f MB/s\r\n", speed);
	}
	else {
		puts("Test size is too small with this case");
	}
#endif /* #if BSP_CFG_DCACHE_ENABLED */
}

#if TEST_SDRAM_EN_AUTO_RF
static uint8_t checkAutoRefresh(void)
{
	uint32_t i;

	uint32_t *p32_cache = (uint32_t *)s_cache;
	uint32_t *p32_sdram = (uint32_t *)s_sdram;

	srand(2021);
	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		p32_cache[i] = (uint32_t)rand();
		p32_sdram[i] = p32_cache[i];
	}
	__DSB();
	__ISB();

	R_BSP_SdramSelfRefreshEnable();
	R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
	R_BSP_SdramSelfRefreshDisable();

	for (i = 0; i < (CACHE_SIZE / 4); i++) {
		if (p32_sdram[i] != p32_cache[i]) {
			printf("Failed at 0x%p. ", &p32_sdram[i]);
			printf("Cache: 0x%08X, SDRAM: 0x%08X\r\n", p32_cache[i], p32_sdram[i]);
			return 1;
		}
	}

	return 0;
}
#endif

#pragma clang diagnostic pop

#endif
