#include "at24c256.h"
#include "hal_data.h"
#include "softiic.h"

#define TAG __FUNCTION__

#define AT24C256_INTERFACE_I2C		1
#define AT24C256_INTERFACE_I2C_SOFT	2
#define AT24C256_INTERFACE_I3C		3

#ifndef __AT24C256_DEBUG
#define __AT24C256_DEBUG	0
#endif

#ifndef __AT24C256_INTERFACE
#define __AT24C256_INTERFACE	AT24C256_INTERFACE_I2C_SOFT
#endif

#if __AT24C256_DEBUG
#include "utils/log.h"
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#define AT24C256_LOGD(msg, ...)			LOG_D(TAG, msg, ##__VA_ARGS__)
#define AT24C256_LOGW(msg, ...)			LOG_W(TAG, msg, ##__VA_ARGS__)
#define AT24C256_LOGE(msg, ...)			LOG_E(TAG, msg, ##__VA_ARGS__)
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#define AT24C256_LOGD(msg, ...)
#define AT24C256_LOGW(msg, ...)
#define AT24C256_LOGE(msg, ...)
#endif

#if __AT24C256_INTERFACE == AT24C256_INTERFACE_I2C_SOFT
static SoftiicType s_iic;
#endif

uint32_t AT24C256_Init(void)
{
#if __AT24C256_INTERFACE == AT24C256_INTERFACE_I2C_SOFT
	s_iic.pin_scl = BSP_IO_PORT_04_PIN_01;
	s_iic.pin_sda = BSP_IO_PORT_04_PIN_00;
	s_iic.time = 2;
#endif
	return 0;
}

uint32_t AT24C256_Read(uint16_t addr, uint8_t *rdata, uint16_t length)
{
	uint32_t ret;

	uint16_t word_addr = addr % 32768;

#if __AT24C256_DEBUG
	LOG_D(TAG, "word_addr: 0x%X", word_addr);
#endif

#if __AT24C256_INTERFACE == AT24C256_INTERFACE_I2C
	ret = IIC_ReadMemory(0x50, word_addr, 16, rdata, length);
#elif __AT24C256_INTERFACE == AT24C256_INTERFACE_I2C_SOFT
	ret = SoftiicReadMem(&s_iic, 0x50, word_addr, 16, rdata, length);
#else
	ret = I3C_ReadMemory(word_addr, rdata, length);
#endif
	UNLIKE_RETURN(ret, 0, "IIC Read failed, %d", ret);

	return 0;
}

uint32_t AT24C256_Write(uint16_t addr, uint8_t *wdata, uint16_t length)
{
	uint32_t ret;

	uint16_t word_addr = addr % 32768;
	uint8_t *pw = wdata;

	while (length) {
		/* Calculate remaining bytes in current page (AT24C256 page size = 64 bytes) */
		uint16_t page_remain = 64 - (word_addr % 64);
		if (length < page_remain) {
			page_remain = length;
		}

#if __AT24C256_DEBUG
		LOG_D(TAG, "word_addr: 0x%X", word_addr);
		LOG_D(TAG, "page_remain: 0x%X", page_remain);
		LOG_D(TAG, "length: %d", length);
		LOG_D(TAG, "pw: %p", pw);
#endif

#if __AT24C256_INTERFACE == AT24C256_INTERFACE_I2C
		ret = IIC_WriteMemory(0x50, word_addr, 16, pw, page_remain);
#elif __AT24C256_INTERFACE == AT24C256_INTERFACE_I2C_SOFT
		ret = SoftiicWriteMem(&s_iic, 0x50, word_addr, 16, pw, page_remain);
#else
		ret = I3C_WriteMemory(word_addr, pw, page_remain);
#endif
		UNLIKE_RETURN(ret, 0, "IIC Write failed, %d", ret);

		/* Wait for EEPROM internal write cycle to complete (AT24C256 max t_WR = 5ms).
		 * Use fixed delay instead of ACK polling to avoid any I2C bus activity
		 * that could interfere with the write cycle. */
		R_BSP_SoftwareDelay(6, BSP_DELAY_UNITS_MILLISECONDS);

		length -= page_remain;
		pw += page_remain;
		word_addr += page_remain;
	}

	return 0;
}

uint32_t AT24C256_Fill(uint16_t addr, uint8_t value, uint16_t length)
{
	uint32_t ret;
	/* AT24C256 page size = 64 bytes, use a page buffer for efficient writes. */
	uint8_t page[64];
	uint16_t i;

	uint16_t word_addr = addr % 32768;

	/* Pre-fill the page buffer with the fill value. */
	for (i = 0; i < 64; i++) {
		page[i] = value;
	}

	while (length) {
		uint16_t page_remain = 64 - (word_addr % 64);
		if (length < page_remain) {
			page_remain = length;
		}

#if __AT24C256_DEBUG
		LOG_D(TAG, "word_addr: 0x%X", word_addr);
		LOG_D(TAG, "page_remain: 0x%X", page_remain);
		LOG_D(TAG, "length: %d", length);
		LOG_D(TAG, "value: 0x%X", value);
#endif

#if __AT24C256_INTERFACE == AT24C256_INTERFACE_I2C
		ret = IIC_WriteMemory(0x50, word_addr, 16, page, page_remain);
#elif __AT24C256_INTERFACE == AT24C256_INTERFACE_I2C_SOFT
		ret = SoftiicWriteMem(&s_iic, 0x50, word_addr, 16, page, page_remain);
#else
		ret = I3C_WriteMemory(word_addr, page, page_remain);
#endif
		UNLIKE_RETURN(ret, 0, "IIC Fill failed, %d", ret);

		/* Wait for EEPROM internal write cycle to complete (AT24C256 max t_WR = 5ms). */
		R_BSP_SoftwareDelay(6, BSP_DELAY_UNITS_MILLISECONDS);

		length -= page_remain;
		word_addr += page_remain;
	}

	return 0;
}
