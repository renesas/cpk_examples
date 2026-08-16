#include "test.h"

#if TEST_EN_SD

#include "hal_data.h"
#include "sd.h"
#include "perf_counter/perf_counter.h"
#include "utils/log.h"

#ifndef TEST_SD_CACHE_SIZE
#define TEST_SD_CACHE_SIZE	(1024 * 64)
#endif

#ifndef TEST_SD_TEST_SIZE
#define TEST_SD_TEST_SIZE	(1024 * 1024)
#endif

#define TAG __FUNCTION__

static uint8_t s_rcache[TEST_SD_CACHE_SIZE];
static uint8_t s_wcache[TEST_SD_CACHE_SIZE];

uint32_t TestSD(void)
{
	uint32_t i, j;
	uint32_t err;
	int64_t time_start, time_end;
	float _t;

	float speed_write = 0.0f;
	float speed_read = 0.0f;
	uint32_t repeat = TEST_SD_TEST_SIZE / TEST_SD_CACHE_SIZE;
	uint32_t block_addr = 0;

	memset(s_rcache, 0x00, TEST_SD_CACHE_SIZE);
	memset(s_wcache, 0xA5, TEST_SD_CACHE_SIZE);

	for (i = 0; i < repeat; i++) {
		time_start = get_system_us();
		err = SD_Write(s_wcache, block_addr, TEST_SD_CACHE_SIZE);
		if (err) {
			LOG_E(TAG, "Write failed: 0x%u. At repeat: %u", err, i);
			return err;
		}
		SD_WaitTrans();
		time_end = get_system_us();
		if (time_end != time_start) {
			_t = (float)TEST_SD_CACHE_SIZE / (float)(time_end - time_start);
			_t = _t * 1000000 / 1024 / 1024;
			speed_write += _t;
		}

		time_start = get_system_us();
		err = SD_Read(s_rcache, block_addr, TEST_SD_CACHE_SIZE);
		if (err) {
			LOG_E(TAG, "Read failed: 0x%u. At repeat: %u", err, i);
			return err;
		}
		SD_WaitTrans();
		time_end = get_system_us();
		for (j = 0; j < TEST_SD_CACHE_SIZE; j++) {
			if (s_rcache[j] != 0xA5) {
				LOG_E(TAG, "Read validate failed. At repeat: %u, pos: %u", i, j);
				return 1;
			}
		}
		if (time_end != time_start) {
			_t = (float)TEST_SD_CACHE_SIZE / (float)(time_end - time_start);
			_t = _t * 1000000 / 1024 / 1024;
			speed_read += _t;
		}

		block_addr += TEST_SD_CACHE_SIZE / 512;
		memset(s_rcache, 0x00, TEST_SD_CACHE_SIZE);
	}

	LOG_D(TAG, "Write speed: %.2f MB/s", speed_write / (float)repeat);
	LOG_D(TAG, "Read  speed: %.2f MB/s", speed_read / (float)repeat);

	return 0;
}

#endif
