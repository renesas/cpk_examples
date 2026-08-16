#include "test.h"

#if TEST_EN_AT24C256

#include "at24c256.h"
#include "hal_data.h"
#include "utils/log.h"

static uint8_t s_rcache[32768];
static uint8_t s_wcache[32768];

uint32_t TestAT24C256(void)
{
	uint16_t i;
	uint32_t ret;

	uint8_t failed = 0;
	uint16_t *p16_r = (uint16_t *)s_rcache;
	uint16_t *p16_w = (uint16_t *)s_wcache;

	for (i = 0; i < 16384; i++) {
    	p16_w[i] = i;
    }
	ret = AT24C256_Write(0x00, s_wcache, 32768);
	if (ret != 0) {
		return ret;
	}
	ret = AT24C256_Read(0x00, s_rcache, 32768);
	if (ret != 0) {
		return ret;
	}
	for (i = 0; i < 16384; i++) {
		if (p16_r[i] != p16_w[i]) {
			failed = 1;
			break;
		}
	}
	if (failed) {
		LOG_E(__FUNCTION__, "WR failed at %d", i);
		LOG_E(__FUNCTION__, "Write: 0x%X, Read: 0x%X", p16_w[i], p16_r[i]);
		return 1;
	}
    LOG_D(__FUNCTION__, "WR pass");

    ret = AT24C256_Fill(0x00, 0xFF, 32768);
    if (ret != 0) {
    	return ret;
    }
    ret = AT24C256_Read(0x00, s_rcache, 32768);
    if (ret != 0) {
    	return ret;
    }
    for (i = 0; i < 32768; i++) {
		if (s_rcache[i] != 0xFF) {
			failed = 1;
			break;
		}
	}
	if (failed) {
		LOG_E(__FUNCTION__, "Erase failed at %d", i);
		LOG_E(__FUNCTION__, "Erase: 0xFF, Read: 0x%X", s_rcache[i]);
		return 1;
	}
    LOG_D(__FUNCTION__, "Erase pass");


	return 0;
}

#endif
