#include <math.h>
#include <string.h>

#include "es8311.h"
#include "hal_data.h"
#include "iic.h"

#ifndef __ES8311_DEBUG
#define __ES8311_DEBUG	1
#endif

#if __ES8311_DEBUG

#include "utils/log.h"

#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#endif

#define ES8311_Read(addr, data, length)	IIC_ReadMemory(ES8311_ADDRESS, addr, 8, data, length)
#define ES8311_Write(data, length)		IIC_Write(ES8311_ADDRESS, data, length)

#define REG_RESET				0x00
/* Clock manager */
#define REG_CLK_MANAGER_01		0x01
#define REG_CLK_MANAGER_02		0x02
#define REG_CLK_MANAGER_03		0x03
#define REG_CLK_MANAGER_04		0x04
#define REG_CLK_MANAGER_05		0x05
#define REG_CLK_MANAGER_06		0x06
#define REG_CLK_MANAGER_07		0x07
#define REG_CLK_MANAGER_08		0x08
/* SDP */
#define REG_SDP_IN				0x09
#define REG_SDP_OUT				0x0A
/* System */
#define REG_SYSTEM_0B			0x0B
#define REG_SYSTEM_0C			0x0C
#define REG_SYSTEM_0D			0x0D
#define REG_SYSTEM_0E			0x0E
#define REG_SYSTEM_0F			0x0F
#define REG_SYSTEM_10			0x10
#define REG_SYSTEM_11			0x11
#define REG_SYSTEM_12			0x12
#define REG_SYSTEM_13			0x13
#define REG_SYSTEM_14			0x14
/* ADC */
#define REG_ADC_15				0x15
#define REG_ADC_16				0x16
#define REG_ADC_17				0x17
#define REG_ADC_18				0x18
#define REG_ADC_19				0x19
#define REG_ADC_1A				0x1A
#define REG_ADC_1B				0x1B
#define REG_ADC_1C				0x1C
#define REG_ADC_1D				0x1D
/* ADCEQ */
#define REG_ADCEQ_1E			0x1E
#define REG_ADCEQ_1F			0x1F
#define REG_ADCEQ_20			0x20
#define REG_ADCEQ_21			0x21
#define REG_ADCEQ_22			0x22
#define REG_ADCEQ_23			0x23
#define REG_ADCEQ_24			0x24
#define REG_ADCEQ_25			0x25
#define REG_ADCEQ_26			0x26
#define REG_ADCEQ_27			0x27
#define REG_ADCEQ_28			0x28
#define REG_ADCEQ_29			0x29
#define REG_ADCEQ_2A			0x2A
#define REG_ADCEQ_2B			0x2B
#define REG_ADCEQ_2C			0x2C
#define REG_ADCEQ_2D			0x2D
#define REG_ADCEQ_2E			0x2E
#define REG_ADCEQ_2F			0x2F
#define REG_ADCEQ_30			0x30
/* DAC */
#define REG_DAC_31				0x31
#define REG_DAC_32				0x32
#define REG_DAC_33				0x33
#define REG_DAC_34				0x34
#define REG_DAC_35				0x35
#define REG_DAC_36				0x36
#define REG_DAC_37				0x37
/* DACEQ */
#define REG_DACEQ_38			0x38
#define REG_DACEQ_39			0x39
#define REG_DACEQ_3A			0x3A
#define REG_DACEQ_3B			0x3B
#define REG_DACEQ_3C			0x3C
#define REG_DACEQ_3D			0x3D
#define REG_DACEQ_3E			0x3E
#define REG_DACEQ_3F			0x3F
#define REG_DACEQ_40			0x40
#define REG_DACEQ_41			0x41
#define REG_DACEQ_42			0x42
#define REG_DACEQ_43			0x43
/* Other */
#define REG_GPIO				0x44
#define REG_GP					0x45
#define REG_IIC					0xFA
#define REG_FLAG				0xFC
#define REG_CHIP_ID_01			0xFD
#define REG_CHIP_ID_02			0xFE
#define REG_CHIP_VERSION		0xFF

/*
 * Clock coefficient structer
 */
struct _coeff_div {
    uint32_t mclk;        /* mclk frequency */
    uint32_t rate;        /* sample rate */
    uint8_t pre_div;      /* the pre divider with range from 1 to 8 */
    uint8_t pre_multi;    /* the pre multiplier with x1, x2, x4 and x8 selection */
    uint8_t adc_div;      /* adcclk divider */
    uint8_t dac_div;      /* dacclk divider */
    uint8_t fs_mode;      /* double speed or single speed, =0, ss, =1, ds */
    uint8_t lrck_h;       /* adclrck divider and daclrck divider */
    uint8_t lrck_l;
    uint8_t bclk_div;     /* sclk divider */
    uint8_t adc_osr;      /* adc osr */
    uint8_t dac_osr;      /* dac osr */
};

/* codec hifi mclk clock divider coefficients */
static const struct _coeff_div sc_coeff_div[] = {
    //mclk     rate   pre_div  mult  adc_div dac_div fs_mode lrch  lrcl  bckdiv osr
    /* 8k */
    {12288000, 8000 , 0x06, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 8000 , 0x03, 0x02, 0x03, 0x03, 0x00, 0x05, 0xff, 0x18, 0x10, 0x10},
    {16384000, 8000 , 0x08, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000 , 8000 , 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000 , 8000 , 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000 , 8000 , 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000 , 8000 , 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000 , 8000 , 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000 , 8000 , 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1024000 , 8000 , 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 11.025k */
    {11289600, 11025, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800 , 11025, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400 , 11025, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200 , 11025, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 12k */
    {12288000, 12000, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000 , 12000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000 , 12000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000 , 12000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 16k */
    {12288000, 16000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 16000, 0x03, 0x02, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
    {16384000, 16000, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000 , 16000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000 , 16000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000 , 16000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000 , 16000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000 , 16000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000 , 16000, 0x03, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1024000 , 16000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 22.05k */
    {11289600, 22050, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800 , 22050, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400 , 22050, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200 , 22050, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 24k */
    {12288000, 24000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 24000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000 , 24000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000 , 24000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000 , 24000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 32k */
    {12288000, 32000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 32000, 0x03, 0x04, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
    {16384000, 32000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000 , 32000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000 , 32000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000 , 32000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000 , 32000, 0x03, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000 , 32000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000 , 32000, 0x03, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
    {1024000 , 32000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 44.1k */
    {11289600, 44100, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800 , 44100, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400 , 44100, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200 , 44100, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 48k */
    {12288000, 48000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 48000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000 , 48000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000 , 48000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000 , 48000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 64k */
    {12288000, 64000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 64000, 0x03, 0x04, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {16384000, 64000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000 , 64000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000 , 64000, 0x01, 0x04, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {4096000 , 64000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000 , 64000, 0x01, 0x08, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {2048000 , 64000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000 , 64000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0xbf, 0x03, 0x18, 0x18},
    {1024000 , 64000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

    /* 88.2k */
    {11289600, 88200, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800 , 88200, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400 , 88200, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200 , 88200, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

    /* 96k */
    {12288000, 96000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 96000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000 , 96000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000 , 96000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000 , 96000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
};

static uint8_t s_tx_done = 1;
static uint8_t s_rx_done = 1;
static ES8311_CallbackFunc s_callback_funcs[ES8311_ON_TRANSMIT_DONE + 1];

static int getCoeff(uint32_t mclk, uint32_t rate);
static void pinConfig(void);

/**
 * @brief  ADC→DAC 内部数字环回，用于诊断麦克风通路
 * @param  enable true=环回开启(ADC数据直接送DAC), false=关闭
 * @note   开启后无需 I2S 即可从扬声器听到麦克风声音
 */
void ES8311_Adc2DacLoopback(bool enable)
{
	uint8_t regv;
	uint8_t tx_cache[2];

	ES8311_Read(REG_GPIO, &regv, 1);
#if __ES8311_DEBUG
	LOG_D(__FUNCTION__, "REG_GPIO: 0x%X", regv);
#endif
	if (enable) {
		regv |= 0x80;
	}
	else {
		regv &= ~0x80;
	}
	tx_cache[0] = REG_GPIO;
	tx_cache[1] = regv;
	ES8311_Write(tx_cache, 2);
}

uint32_t ES8311_Init(void)
{
	uint8_t datmp;
	uint8_t rx_cache;
	uint8_t tx_cache[2];
	uint32_t err;
	uint32_t rate, mclk;
	int coe_index;

	for (coe_index = 0; coe_index < (ES8311_ON_TRANSMIT_DONE + 1); coe_index++) {
		s_callback_funcs[coe_index] = NULL;
	}

	pinConfig();

	err = ES8311_Read(REG_CHIP_VERSION, &rx_cache, 1);
	UNLIKE_RETURN(err, 0, "Read version failed");
#if __ES8311_DEBUG
	LOG_D(__FUNCTION__, "Version: 0x%X", rx_cache);
#endif
	err = ES8311_Read(REG_CHIP_ID_01, &rx_cache, 1);
	UNLIKE_RETURN(err, 0, "Read ID_01 failed");
#if __ES8311_DEBUG
	LOG_D(__FUNCTION__, "ID_01: 0x%X", rx_cache);
#endif
	err = ES8311_Read(REG_CHIP_ID_02, &rx_cache, 1);
	UNLIKE_RETURN(err, 0, "Read ID_02 failed");
#if __ES8311_DEBUG
	LOG_D(__FUNCTION__, "ID_02: 0x%X", rx_cache);
#endif

	tx_cache[0] = REG_CLK_MANAGER_01;
	tx_cache[1] = 0x30;
	err = ES8311_Write(tx_cache, 2);
	UNLIKE_RETURN(err, 0, "Write REG_CLK_MANAGER_01 failed");
	err = ES8311_Read(REG_CLK_MANAGER_01, &rx_cache, 1);
	UNLIKE_RETURN(err, 0, "Read REG_CLK_MANAGER_01 failed");
	if (rx_cache != tx_cache[1]) {
	#if __ES8311_DEBUG
		LOG_E(__FUNCTION__, "REG_CLK_MANAGER_01 write[0x%X] != read[0x%X]", tx_cache[1], rx_cache);
	#endif
		return FSP_ERR_ASSERTION;
	}

	tx_cache[0] = REG_CLK_MANAGER_02;
	tx_cache[1] = 0x00;
	ES8311_Write(tx_cache, 2);
	tx_cache[0] = REG_CLK_MANAGER_03;
	tx_cache[1] = 0x10;
	ES8311_Write(tx_cache, 2);
	tx_cache[0] = REG_ADC_16;
	tx_cache[1] = 0x24;
	ES8311_Write(tx_cache, 2);
	tx_cache[0] = REG_CLK_MANAGER_04;
	tx_cache[1] = 0x10;
	ES8311_Write(tx_cache, 2);
	tx_cache[0] = REG_CLK_MANAGER_05;
	tx_cache[1] = 0x00;
	ES8311_Write(tx_cache, 2);
	tx_cache[0] = REG_SYSTEM_0B;
	tx_cache[1] = 0x00;
	ES8311_Write(tx_cache, 2);
	tx_cache[0] = REG_SYSTEM_0C;
	tx_cache[1] = 0x00;
	ES8311_Write(tx_cache, 2);
	tx_cache[0] = REG_SYSTEM_10;
	tx_cache[1] = 0x1F;
	ES8311_Write(tx_cache, 2);
	tx_cache[0] = REG_SYSTEM_11;
	tx_cache[1] = 0x7F;
	ES8311_Write(tx_cache, 2);
	tx_cache[0] = REG_RESET;
	tx_cache[1] = 0x00;
	ES8311_Write(tx_cache, 2);

	/* Set ES8311 M/S mode depent on FSP configuration.xml */
	ES8311_Read(REG_RESET, &rx_cache, 1);
#if __ES8311_DEBUG
	LOG_D(__FUNCTION__, "REG_RESET: 0x%X", rx_cache);
#endif
	if (g_i2s0_cfg.operating_mode == I2S_MODE_MASTER) {
	#if __ES8311_DEBUG
		LOG_D(__FUNCTION__, "RA8P1 set as Master");
	#endif
		rx_cache &= 0xBF;
	}
	else {
	#if __ES8311_DEBUG
		LOG_D(__FUNCTION__, "RA8P1 set as Slave");
	#endif
		rx_cache |= 0x40;
	}
	tx_cache[0] = REG_RESET;
	tx_cache[1] = rx_cache;
	ES8311_Write(tx_cache, 2);

	/* Set ES8311 MCLK come from MCLK, generated by TIMER 2 */
	tx_cache[0] = REG_CLK_MANAGER_01;
	tx_cache[1] = 0x3F;
	ES8311_Write(tx_cache, 2);

	/* Set ES8311 clock */
	rate = 44100;
	mclk = 11289600;
	coe_index = getCoeff(mclk, rate);
	if (coe_index < 0) {
	#if __ES8311_DEBUG
		LOG_E(__FUNCTION__, "Unable to configure sample rate [%u] Hz with MCLK [%u] Hz", rate, mclk);
	#endif
		return FSP_ERR_ASSERTION;
	}
	ES8311_Read(REG_CLK_MANAGER_02, &rx_cache, 1);
	rx_cache &= 0x07;
	rx_cache |= (sc_coeff_div[coe_index].pre_div - 1) << 5;
	switch (sc_coeff_div[coe_index].pre_multi) {
	case 2:
		datmp = 1;
		break;
	case 4:
		datmp = 2;
		break;
	case 8:
		datmp = 3;
		break;
	default:
		datmp = 0;
		break;
	}
	rx_cache |= (datmp << 3);
	tx_cache[0] = REG_CLK_MANAGER_02;
	tx_cache[1] = rx_cache;
	ES8311_Write(tx_cache, 2);

	tx_cache[0] = REG_CLK_MANAGER_05;
	tx_cache[1] = (uint8_t)((sc_coeff_div[coe_index].adc_div - 1) << 4);
	tx_cache[1] |= (sc_coeff_div[coe_index].dac_div - 1);
	ES8311_Write(tx_cache, 2);

	ES8311_Read(REG_CLK_MANAGER_03, &rx_cache, 1);
	tx_cache[0] = REG_CLK_MANAGER_03;
	tx_cache[1] = rx_cache & 0x80;
	tx_cache[1] |= (sc_coeff_div[coe_index].fs_mode << 6);
	tx_cache[1] |= sc_coeff_div[coe_index].adc_osr;
	ES8311_Write(tx_cache, 2);

	ES8311_Read(REG_CLK_MANAGER_04, &rx_cache, 1);
	tx_cache[0] = REG_CLK_MANAGER_04;
	tx_cache[1] = rx_cache & 0x80;
	tx_cache[1] |= sc_coeff_div[coe_index].dac_osr;
	ES8311_Write(tx_cache, 2);

	ES8311_Read(REG_CLK_MANAGER_07, &rx_cache, 1);
	tx_cache[0] = REG_CLK_MANAGER_07;
	tx_cache[1] = rx_cache & 0xC0;
	tx_cache[1] |= sc_coeff_div[coe_index].lrck_h;
	ES8311_Write(tx_cache, 2);

	tx_cache[0] = REG_CLK_MANAGER_08;
	tx_cache[1] = sc_coeff_div[coe_index].lrck_l;
	ES8311_Write(tx_cache, 2);

	ES8311_Read(REG_CLK_MANAGER_06, &rx_cache, 1);
	tx_cache[0] = REG_CLK_MANAGER_06;
	tx_cache[1] = rx_cache & 0xE0;
	if (sc_coeff_div[coe_index].bclk_div < 19) {
		tx_cache[1] |= (sc_coeff_div[coe_index].bclk_div - 1);
	}
	else {
		tx_cache[1] |= sc_coeff_div[coe_index].bclk_div;
	}
	ES8311_Write(tx_cache, 2);

	tx_cache[0] = REG_SYSTEM_13;
	tx_cache[1] = 0x10;
	ES8311_Write(tx_cache, 2);

	tx_cache[0] = REG_ADC_1B;
	tx_cache[1] = 0x0A;
	ES8311_Write(tx_cache, 2);

	tx_cache[0] = REG_ADC_1C;
	tx_cache[1] = 0x6A;
	ES8311_Write(tx_cache, 2);

	/* IO Config */
	ES8311_Read(REG_GPIO, &rx_cache, 1);
#if __ES8311_DEBUG
	LOG_D(__FUNCTION__, "Read REG_GPIO: 0x%X", rx_cache);
#endif
	tx_cache[0] = REG_GPIO;
	tx_cache[1] = 0x08;
	ES8311_Write(tx_cache, 2);
	tx_cache[0] = REG_GP;
	tx_cache[1] = 0x00;
	ES8311_Write(tx_cache, 2);

	/* Configure SDP input: I2S format, 16-bit word length (matches SSI config) */
	tx_cache[0] = REG_SDP_IN;
	tx_cache[1] = 0x0C;            /* SDP_IN_FMT=I2S, SDP_IN_WL=16-bit */
	ES8311_Write(tx_cache, 2);

	/* Configure SDP output: I2S format, word length */
	tx_cache[0] = REG_SDP_OUT;
	tx_cache[1] = 0x0C;
	ES8311_Write(tx_cache, 2);

	/* Select ADC input channel and set PGA gain */
	tx_cache[0] = REG_SYSTEM_14;
	tx_cache[1] = 0x1A;
	ES8311_Write(tx_cache, 2);

	/* Set ES8311 analog power. ADC for MIC and DAC for speaker */
	tx_cache[0] = REG_SYSTEM_0D;
	tx_cache[1] = 0x01;
	ES8311_Write(tx_cache, 2);
	R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
	tx_cache[1] = 0x06;
	ES8311_Write(tx_cache, 2);
	/* Enable analog PGA and ADC modulator */
	ES8311_Read(REG_SYSTEM_0E, &rx_cache, 1);
#if __ES8311_DEBUG
	LOG_D(__FUNCTION__, "REG_SYSTEM_0E: 0x%X", rx_cache);
#endif
	tx_cache[0] = REG_SYSTEM_0E;
	tx_cache[1] = rx_cache & 0x9F;
	ES8311_Write(tx_cache, 2);

	tx_cache[0] = REG_ADC_15;
	tx_cache[1] = 0x40;
	ES8311_Write(tx_cache, 2);

	/* Set ADC volumn */
	tx_cache[0] = REG_ADC_17;
	tx_cache[1] = 0xBF;
	ES8311_Write(tx_cache, 2);

	tx_cache[0] = REG_SYSTEM_12;
	tx_cache[1] = 0x01;
	ES8311_Write(tx_cache, 2);

	/* DAC volume */
	tx_cache[0] = REG_DAC_32;
	tx_cache[1] = 0xBF;
	ES8311_Write(tx_cache, 2);

	tx_cache[0] = REG_DAC_37;
	tx_cache[1] = 0x48;
	ES8311_Write(tx_cache, 2);

	/* CSM power on */
	ES8311_Read(REG_RESET, &rx_cache, 1);
#if __ES8311_DEBUG
	LOG_D(__FUNCTION__, "Second read REG_RESET: 0x%X", rx_cache);
#endif
    tx_cache[0] = REG_RESET;
    tx_cache[1] = rx_cache | 0x80;
    ES8311_Write(tx_cache, 2);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

	R_SSI_Open(g_i2s0.p_ctrl, g_i2s0.p_cfg);
	R_GPT_Open(g_timer2.p_ctrl, g_timer2.p_cfg);
	R_GPT_Start(g_timer2.p_ctrl);

	return err;
}

ES8311_CallbackFunc ES8311_RegisterCb(ES8311_CallbackFunc cb, ES8311_CallbackEnum cbe)
{
	ES8311_CallbackFunc old = s_callback_funcs[cbe];

	s_callback_funcs[cbe] = cb;

	return old;
}

uint32_t ES8311_Receive(void *data, uint32_t length)
{
	uint32_t err;

	while (s_rx_done == 0) {
		R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
	}

	err = R_SSI_Read(g_i2s0.p_ctrl, data, length);
	UNLIKE_RETURN(err, 0, "R_SSI_Read() failed: %u", err);
	s_rx_done = 0;

	return err;
}

/**
 * @brief  打印 ADC 相关寄存器值，用于诊断模拟通路
 */
void ES8311_DumpAdcRegs(void)
{
	static const uint8_t sc_regs[] = {
		0x00, 0x01, 0x03, 0x04, 0x05,    /* Reset, Clock */
		0x09, 0x0A,                        /* SDP in/out */
		0x0D, 0x0E,                        /* Analog power, PGA */
		0x14,                              /* LINSEL, PGAGAIN */
		0x15, 0x16, 0x17,                  /* ADC ramp, scale, volume */
		0x31, 0x32,                        /* DAC mute, volume */
		0x44,                              /* GPIO */
	};
	uint8_t i, val;

	printf("ES8311 ADC-path register dump:\r\n");
	for (i = 0; i < sizeof(sc_regs); i++) {
		ES8311_Read(sc_regs[i], &val, 1);
		printf("  REG[0x%02X] = 0x%02X\r\n", sc_regs[i], val);
	}
}

/**
 * @brief  设置 DAC 输出音量
 * @param  percent 音量百分比 [0, 100], 100 = 0dB (满幅)
 * @note   DAC 寄存器 0xBF 对应 0dB, 0x00 对应 -95.5dB
 *         100% → 0xBF, 按线性映射到寄存器值
 */
void ES8311_SetVolume(uint8_t percent)
{
	uint8_t vol;

	if (percent >= 100) {
		vol = 0xBF;
	}
	else {
		vol = (uint8_t)((uint32_t)0xBF * percent / 100);
	}

	uint8_t tx_cache[2];
	tx_cache[0] = REG_DAC_32;
	tx_cache[1] = vol;
	ES8311_Write(tx_cache, 2);
}

uint32_t ES8311_Transmit(void *data, uint32_t length)
{
	uint32_t err;

	while (s_tx_done == 0) {
		R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
	}

	err = R_SSI_Write(g_i2s0.p_ctrl, data, length);
	UNLIKE_RETURN(err, 0, "R_SSI_Write() failed: %u", err);
	s_tx_done = 0;

	return err;
}

uint32_t ES8311_WriteRead(void *tx_data, void *rx_data, uint32_t length)
{
	uint32_t err;

	while (s_tx_done == 0 || s_rx_done == 0) {
		R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
	}

	err = R_SSI_WriteRead(g_i2s0.p_ctrl, tx_data, rx_data, length);
	UNLIKE_RETURN(err, 0, "R_SSI_WriteRead() failed: %u", err);
	s_tx_done = 0;
	s_rx_done = 0;

	return err;
}

void I2S_Callback(i2s_callback_args_t *p_args)
{
	switch (p_args->event) {
	case I2S_EVENT_TX_EMPTY:
		s_tx_done = 1;
		if (s_callback_funcs[ES8311_ON_TRANSMIT_DONE] != NULL) {
			s_callback_funcs[ES8311_ON_TRANSMIT_DONE]();
		}
		break;
	case I2S_EVENT_RX_FULL:
		s_rx_done = 1;
		if (s_callback_funcs[ES8311_ON_RECEIVE_DONE] != NULL) {
			s_callback_funcs[ES8311_ON_RECEIVE_DONE]();
		}
		break;
	case I2S_EVENT_IDLE:
		s_tx_done = 1;
		s_rx_done = 1;
		if (s_callback_funcs[ES8311_ON_IDLE] != NULL) {
			s_callback_funcs[ES8311_ON_IDLE]();
		}
		break;
	default:
		break;
	}
}

static int getCoeff(uint32_t mclk, uint32_t rate)
{
	int i = 0;
	int coeff_length = (int)(sizeof(sc_coeff_div) / sizeof(sc_coeff_div[0]));
	for (i = 0; i < coeff_length; i++) {
		if ((sc_coeff_div[i].rate == rate) && (sc_coeff_div[i].mclk == mclk)) {
			return i;
		}
	}

	return -1;
}

static void pinConfig(void)
{
	ioport_instance_ctrl_t pin_ctrl;
	ioport_pin_cfg_t pin_cfg_data[5];
    ioport_cfg_t pin_cfg;

    pin_cfg_data[0].pin = BSP_IO_PORT_04_PIN_03;
    pin_cfg_data[0].pin_cfg = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SSI;
    pin_cfg_data[1].pin = BSP_IO_PORT_04_PIN_04;
    pin_cfg_data[1].pin_cfg = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SSI;
    pin_cfg_data[2].pin = BSP_IO_PORT_04_PIN_05;
    pin_cfg_data[2].pin_cfg = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SSI;
    pin_cfg_data[3].pin = BSP_IO_PORT_04_PIN_06;
    pin_cfg_data[3].pin_cfg = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SSI;

    pin_cfg_data[4].pin = BSP_IO_PORT_13_PIN_06;
    pin_cfg_data[4].pin_cfg = (uint32_t)IOPORT_CFG_DRIVE_MID | (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_GPT1;

    pin_cfg.number_of_pins = 5;
    pin_cfg.p_pin_cfg_data = (ioport_pin_cfg_t const *)&pin_cfg_data;
    pin_cfg.p_extend = NULL;
    R_IOPORT_Open(&pin_ctrl, &pin_cfg);
}
