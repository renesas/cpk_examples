/*============================ INCLUDES ======================================*/
#if !defined(__ARMCOMPILER_VERSION)
#include <byteswap.h>
#endif

#include "hal_data.h"
#include "lcd.h"
#include "r_sci_b_spi_cfg.h"

#include "perf_counter/perf_counter.h"
#include "utils/log.h"

/*============================ MACROS ========================================*/

#define ST7796U_INTERFACE_8080_18BIT        0x00
#define ST7796U_INTERFACE_8080_09BIT        0x01
#define ST7796U_INTERFACE_8080_16BIT        0x02
#define ST7796U_INTERFACE_8080_08BIT        0x03
#define ST7796U_INTERFACE_3SPI              0x05
#define ST7796U_INTERFACE_MIPI              0x06
#define ST7796U_INTERFACE_4SPI              0x07
#define ST7796U_INTERFACE_RGB               0x10

#define UNLIKE_RETURN(v, t, msg, ...) do { if (v != t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return; } } while(0)

#ifndef __ST7796U_INTERFACE
#define __ST7796U_INTERFACE                 ST7796U_INTERFACE_MIPI
#endif

#define ST7796U_PIN_RESET		BSP_IO_PORT_13_PIN_07

#if __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI

#ifndef ST7796U_4SPI_EN_CACHE
#define ST7796U_4SPI_EN_CACHE           1
#endif

#if ST7796U_4SPI_EN_CACHE

#ifndef ST7796U_4SPI_EN_DOUBLE_CACHE
/* NOTE: Unimplemented, don't use */
#define ST7796U_4SPI_EN_DOUBLE_CACHE    0
#endif

#ifndef ST7796U_CACHE_SIZE
#define ST7796U_CACHE_SIZE              (222 * 480 * 2)
#endif

#endif

#define DEFAULT_TIMEOUT     	200
#define ST7796U_SPI_INSTANCE	g_sci_spi8
#define ST7796U_SPI_CALLBACK	SCI8_SPI_Callback
#define ST7796U_SPI_HARD_CS		1
#define ST7796U_PIN_SPI_CLK		BSP_IO_PORT_13_PIN_01
#define ST7796U_PIN_SPI_MOSI	BSP_IO_PORT_13_PIN_02
#define ST7796U_PIN_SPI_MISO	BSP_IO_PORT_13_PIN_03
#define ST7796U_PIN_SPI_CS      BSP_IO_PORT_13_PIN_04
#define ST7796U_PIN_DCX         BSP_IO_PORT_13_PIN_05

#endif /* #if __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI */

/*============================ TYPES =========================================*/
#if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI
typedef struct lcd_mipi_table_setting
{
    unsigned char size;
    unsigned char buffer[15];
    mipi_cmd_id_t cmd_id;
    mipi_dsi_cmd_flag_t flags;
} lcd_mipi_table_setting_t;
#elif __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI
typedef struct lcd_spi_table_setting {
    const uint8_t *cmd;
    uint8_t length;
    uint8_t val[16];
} lcd_spi_table_setting_t;

typedef enum lcd_spi_clk_select {
    SPI_CLK_READ,
    SPI_CLK_WRITE
} lcd_spi_clk_select_t;
#endif

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
#if ST7796U_SPI_HARD_CS == 0
static uint8_t s_spi_sequence_end;
#endif

#if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI
static uint8_t s_mipi_tx_done;
static uint8_t s_glcdc_vsync_flag;
/* This table of commands was adapted from sample code provided by FocusLCD
 * Page Link: https://focuslcds.com/product/4-5-tft-display-capacitive-tp-e45ra-mw276-c/
 * File Link: https://focuslcds.com/content/E45RA-MW276-C_init_code.txt */
static const lcd_mipi_table_setting_t sc_lcd_mipi_init_table[] = {
    {2, {0x11, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0xF0, 0xC3}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0xF0, 0x96}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0x36, 0x48}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0x3A, 0x55}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0xB4, 0x01}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {4, {0xB6, 0x8A, 0x07, 0x3B}, MIPI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0xB7, 0xC6}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {3, {0xB9, 0x02, 0xE0}, MIPI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {3, {0xC0, 0xC0, 0x64}, MIPI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0xC1, 0x1D}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0xC2, 0xA7}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0xC5, 0x18}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {9, {0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}, MIPI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {15, {0xE0, 0xF0, 0x0B, 0x12, 0x09, 0x0A, 0x26, 0x39, 0x54, 0x4E, 0x38, 0x13, 0x13, 0x2E, 0x34}, MIPI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {15, {0xE1, 0xF0, 0x10, 0x15, 0x0D, 0x0C, 0x07, 0x38, 0x43, 0x4D, 0x3A, 0x16, 0x15, 0x30, 0x35}, MIPI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0xF0, 0x3C}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0xF0, 0x69}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0x35, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0x29, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0x21, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {5, {0x2A, 0x00, 0x31, 0x01, 0x0E}, MIPI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {5, {0x2B, 0x00, 0x00, 0x01, 0xDF}, MIPI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2, {0x2C, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
};
#elif __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI

static uint8_t s_spi_transfer_done = 1;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-const-variable"
#endif
/* Command Table 1 */
static const uint8_t sc_cmd_nop = 0x00;
static const uint8_t sc_cmd_swreset = 0x01;
static const uint8_t sc_cmd_rddid = 0x04;
static const uint8_t sc_cmd_read_num_of_dsi_err = 0x05;
static const uint8_t sc_cmd_rddst = 0x09;
static const uint8_t sc_cmd_rddpm = 0x0A;
static const uint8_t sc_cmd_rddmadctl = 0x0B;
static const uint8_t sc_cmd_rddcolmod = 0x0C;
static const uint8_t sc_cmd_rddim = 0x0D;
static const uint8_t sc_cmd_rddsm = 0x0E;
static const uint8_t sc_cmd_rddsdr = 0x0F;
static const uint8_t sc_cmd_slpin = 0x10;
static const uint8_t sc_cmd_slpout = 0x11;
static const uint8_t sc_cmd_ptlon = 0x12;
static const uint8_t sc_cmd_noron = 0x13;
static const uint8_t sc_cmd_invoff = 0x20;
static const uint8_t sc_cmd_invon = 0x21;
static const uint8_t sc_cmd_dispoff = 0x28;
static const uint8_t sc_cmd_dispon = 0x29;
static const uint8_t sc_cmd_caset = 0x2A;
static const uint8_t sc_cmd_raset = 0x2B;
static const uint8_t sc_cmd_ramwr = 0x2C;
static const uint8_t sc_cmd_ramrd = 0x2E;
static const uint8_t sc_cmd_ptlar = 0x30;
static const uint8_t sc_cmd_vscrdef = 0x33;
static const uint8_t sc_cmd_teoff = 0x34;
static const uint8_t sc_cmd_teon = 0x35;
static const uint8_t sc_cmd_madctl = 0x36;
static const uint8_t sc_cmd_vscsad = 0x37;
static const uint8_t sc_cmd_idmoff = 0x38;
static const uint8_t sc_cmd_idmon = 0x39;
static const uint8_t sc_cmd_colmod = 0x3A;
static const uint8_t sc_cmd_wrmemc = 0x3C;
static const uint8_t sc_cmd_rdmemc = 0x3E;
static const uint8_t sc_cmd_ste = 0x44;
static const uint8_t sc_cmd_gscan = 0x45;
static const uint8_t sc_cmd_wrdisbv = 0x51;
static const uint8_t sc_cmd_rddisbv = 0x52;
static const uint8_t sc_cmd_wrctrld = 0x53;
static const uint8_t sc_cmd_rdctrld = 0x54;
static const uint8_t sc_cmd_wrcabc = 0x55;
static const uint8_t sc_cmd_rdcabc = 0x56;
static const uint8_t sc_cmd_wrcabcmb = 0x5E;
static const uint8_t sc_cmd_rdcabcmb = 0x5F;
static const uint8_t sc_cmd_rdfcs = 0xAA;
static const uint8_t sc_cmd_rdcfcs = 0xAF;
static const uint8_t sc_cmd_rdid1 = 0xDA;
static const uint8_t sc_cmd_rdid2 = 0xDB;
static const uint8_t sc_cmd_rdid3 = 0xDC;
/* Command Table 2 */
static const uint8_t sc_cmd_ifmode = 0xB0;
static const uint8_t sc_cmd_frmctr1 = 0xB1;
static const uint8_t sc_cmd_frmctr2 = 0xB2;
static const uint8_t sc_cmd_frmctr3 = 0xB3;
static const uint8_t sc_cmd_dic = 0xB4;
static const uint8_t sc_cmd_bpc = 0xB6;
static const uint8_t sc_cmd_em = 0xB7;
static const uint8_t sc_cmd_modesel = 0xB9;
static const uint8_t sc_cmd_pwr1 = 0xC0;
static const uint8_t sc_cmd_pwr2 = 0xC1;
static const uint8_t sc_cmd_pwr3 = 0xC2;
static const uint8_t sc_cmd_vcmpctl = 0xC5;
static const uint8_t sc_cmd_vcm_offset = 0xC6;
static const uint8_t sc_cmd_nvmadw = 0xD0;
static const uint8_t sc_cmd_nvmbprog = 0xD1;
static const uint8_t sc_cmd_nvm_status_read = 0xD2;
static const uint8_t sc_cmd_rdid4 = 0xD3;
static const uint8_t sc_cmd_pgc = 0xE0;
static const uint8_t sc_cmd_ngc = 0xE1;
static const uint8_t sc_cmd_dgc1 = 0xE2;
static const uint8_t sc_cmd_dgc2 = 0xE3;
static const uint8_t sc_cmd_doca = 0xE8;
static const uint8_t sc_cmd_cscon = 0xF0;
static const uint8_t sc_cmd_spi_read_control = 0xFB;

/* If lcd_spi_table_setting_t::cmd is NULL, that means delay lcd_spi_table_setting_t::length ms */
static const lcd_spi_table_setting_t sc_lcd_setting_table[] = {
    {NULL           , 120, {0x00}},
    {&sc_cmd_slpout , 0  , {0x00}},
    {NULL           , 120, {0x00}},
    {&sc_cmd_cscon  , 1  , {0xC3}},
    {&sc_cmd_cscon  , 1  , {0x96}},
    {&sc_cmd_madctl , 1  , {0x48}},
    {&sc_cmd_colmod , 1  , {0x55}},
    {&sc_cmd_dic    , 1  , {0x01}},
    {&sc_cmd_bpc    , 3  , {0x8A, 0x07, 0x3B}},
    {&sc_cmd_em     , 1  , {0xC6}},
    {&sc_cmd_modesel, 2  , {0x02, 0xE0}},
    {&sc_cmd_pwr1   , 2  , {0xC0, 0x64}},
    {&sc_cmd_pwr2   , 1  , {0x1D}},
    {&sc_cmd_pwr3   , 1  , {0xA7}},
    {&sc_cmd_vcmpctl, 1  , {0x18}},
    {&sc_cmd_doca   , 8  , {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}},
    {&sc_cmd_pgc    , 14 , {0xF0, 0x0B, 0x12, 0x09, 0x0A, 0x26, 0x39, 0x54, 0x4E, 0x38, 0x13, 0x13, 0x2E, 0x34}},
    {&sc_cmd_ngc    , 14 , {0xF0, 0x10, 0x15, 0x0D, 0x0C, 0x07, 0x38, 0x43, 0x4D, 0x3A, 0x16, 0x15, 0x30, 0x35}},
    {&sc_cmd_teon   , 1  , {0x00}},
    {&sc_cmd_invon  , 0  , {0x00}},
    {&sc_cmd_caset  , 4  , {0x00, 0x00, 0x01, 0x0E}},
    {&sc_cmd_raset  , 4  , {0x00, 0x00, 0x01, 0xDF}},
	{&sc_cmd_cscon  , 1  , {0x3C}},
    {&sc_cmd_cscon  , 1  , {0x69}},
	{&sc_cmd_dispon , 0  , {0x00}},
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/* When SCICLK is 120MHz, this config set SCI_SPI clock to 6MHz.
 * This clock config is for reading, must less then 6.66MHz */
static const sci_b_spi_extended_cfg_t sc_spi_read_clk_config = {
    .clk_div = { .cks = 0, .brr = 9, .bgdm = 1 },
    .clock_source = SCI_B_SPI_SOURCE_CLOCK_SCISPICLK
};

/* It will copy from FSP generation when call LCD_PortInit.
 * This clock config is for writing. */
static sci_b_spi_extended_cfg_t s_spi_write_clk_config;

#if ST7796U_4SPI_EN_CACHE
#if ST7796U_4SPI_EN_DOUBLE_CACHE
static uint8_t s_spi_frame_cache[2][ST7796U_CACHE_SIZE];
static uint8_t s_spi_frame_current;
#else
static uint8_t s_spi_frame_cache[1][ST7796U_CACHE_SIZE];
#endif
#endif

#endif /* #if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI */

/*============================ PROTOTYPES ====================================*/
#pragma clang diagnostic ignored "-Wunused-function"

static void hardReset(void);

#if __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI
static void readMemory(uint8_t *val, uint32_t length);
static void readRegister(const uint8_t *reg, uint8_t *val, uint32_t length);
static void setClock(lcd_spi_clk_select_t select);
static void setCursor(LCD_Device *device, uint16_t x, uint16_t y);
static void setRenderArea(LCD_Device *device, uint16_t x, uint16_t y, uint16_t length, uint16_t width);
static void waitTransferDone(uint32_t timeout_ms);
static void writeMemory(const uint8_t *val, uint32_t length);
static void writeRegister(const uint8_t *reg, const uint8_t *val, uint32_t length);
#endif

/*============================ PUBLIC FUNCTIONS ==============================*/
uint32_t LCD_PortDrawBitmap(LCD_Device *device, uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t *bitmap)
{
	uint16_t src_length = length;

	if ((length == 0) || (width == 0)) {
        return 0;
    }
	if ((x >= device->width) || (y >= device->high)) {
		LOG_W(__FUNCTION__, "[%u, %u] out of range", x, y);
		return 1;
	}
	if ((x + length) > device->width) {
		length = device->width - x;
	}
	if ((y + width) > device->high) {
		width = device->high - y;
	}

	LOG_D(__FUNCTION__, "x = %u, y = %u, length = %u, width = %u", x, y, length, width);

#if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI
	(void)device;
	(void)x;
	(void)y;
	(void)length;
	(void)width;
	(void)bitmap;
#elif __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI
	uint16_t i, j;

	const uint16_t *p16_bitmap = (const uint16_t *)bitmap;

#if ST7796U_4SPI_EN_CACHE

	uint16_t *p16_cache = (uint16_t *)&s_spi_frame_cache[0];
	uint32_t index = x + y * device->width;

	for (i = 0; i < width; i++) {
		for (j = 0; j < length; j++) {
		#if defined(__ARMCOMPILER_VERSION)
			p16_cache[index + j] = p16_bitmap[i * src_length + j] >> 8;
			p16_cache[index + j] |= p16_bitmap[i * src_length + j] << 8;
		#else
			p16_cache[index + j] = bswap_16(p16_bitmap[i * src_length + j]);
		#endif
		}
		index += device->width;
	}

	setCursor(device, 0, y);
	writeRegister(&sc_cmd_ramwr, NULL, 0);
    waitTransferDone(DEFAULT_TIMEOUT);
#if ST7796U_SPI_HARD_CS == 0
    s_spi_sequence_end = 1;
#endif
    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_HIGH);
    R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_LOW);
    R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, &p16_cache[y * device->width], width * device->width * 2, SPI_BIT_WIDTH_8_BITS);
#else
    uint16_t color;

    setRenderArea(device, x, y, length, width);
    writeRegister(&sc_cmd_ramwr, NULL, 0);
    waitTransferDone(DEFAULT_TIMEOUT);
    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_HIGH);
    R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_LOW);
    for (i = 0; i < width; i++) {
    	for (j = 0; j < length; j++) {
		#if defined(__ARMCOMPILER_VERSION)
			color = p16_bitmap[i * src_length + j] >> 8;
			color |= p16_bitmap[i * src_length + j] << 8;
		#else
			color = bswap_16(p16_bitmap[i * src_length + j]);
		#endif
			R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, &color, 2, SPI_BIT_WIDTH_8_BITS);
			waitTransferDone(DEFAULT_TIMEOUT);
    	}
    }
    R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_HIGH);
#endif

#endif
	return 0;
}

uint32_t LCD_PortDrawPoint(LCD_Device *device, uint16_t x, uint16_t y, uint32_t color)
{
	if ((x >= device->width) || (y >= device->high)) {
		return 1;
	}

#if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI
	(void)device;
	(void)x;
	(void)y;
	(void)color;
#elif __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI

#if ST7796U_4SPI_EN_CACHE
	uint16_t *p16_cache = NULL;
	uint32_t *p32_cache = NULL;
	uint32_t index = x + y * device->width;

	setCursor(device, x, y);
	if (device->color_format == COLOR_FORMAT_RGB565) {
		p16_cache = (uint16_t *)s_spi_frame_cache;
		p16_cache[index] = (uint16_t)color;
		writeMemory((uint8_t *)&p16_cache[index], 2);
	}
	else {
		p32_cache = (uint32_t *)s_spi_frame_cache;
		p32_cache[index] = color;
		writeMemory((uint8_t *)&p32_cache[index], 4);
	}
#else
	uint16_t color_16bit;
	uint32_t color_32bit;

	setCursor(device, x, y);
	if (device->color_format == COLOR_FORMAT_RGB565) {
		color_16bit = (uint16_t)color;
		writeMemory((uint8_t *)&color_16bit, 2);
	}
	else {
		color_32bit = color;
		writeMemory((uint8_t *)&color_32bit, 4);
	}
	waitTransferDone(DEFAULT_TIMEOUT);
#endif

#endif /* #if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI */

	return 0;
}

uint32_t LCD_PortFillRectangle(LCD_Device *device, uint16_t x, uint16_t y, uint16_t length, uint16_t width, uint32_t color)
{
    if ((length == 0) || (width == 0)) {
    	LOG_W(__FUNCTION__, "Value chacke failed: length = %u, width = %u", length, width);
        return 0;
    }
	if ((x >= device->width) || (y >= device->high)) {
		LOG_W(__FUNCTION__, "Value chacke failed: x = %u, y = %u", x, y);
		return 1;
	}
	if ((x + length) > device->width) {
		length = device->width - x;
	}
	if ((y + width) > device->high) {
		width = device->high - y;
	}

	LOG_D(__FUNCTION__, "x = %u, y = %u, length = %u, width = %u, color = 0x%04X", x, y, length, width, color);

#if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI
	(void)device;
	(void)x;
	(void)y;
	(void)length;
	(void)width;
	(void)color;
#elif __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI

#if ST7796U_4SPI_EN_CACHE
	uint16_t _color;
    uint32_t i, j;
    uint32_t index;

    uint16_t row_num = 0;
    uint16_t *p16_cache = NULL;
    uint32_t *p32_cache = NULL;

    if (device->color_format == COLOR_FORMAT_RGB565) {
	#if defined(__ARMCOMPILER_VERSION)
		_color = (uint16_t)((color >> 8) & 0xFF);
		_color |= (uint16_t)((color << 8) & 0xFF00);
	#else
		_color = bswap_16((uint16_t)color);
	#endif
    	index = x + y * device->width;
    	p16_cache = (uint16_t *)s_spi_frame_cache;
        for (i = 0; i < width; i++) {
            for (j = 0; j < length; j++) {
            	p16_cache[index + j] = _color;
            }
            index += device->width;
            row_num++;
        }
    }
    else {
    	index = x + y * device->width;
        p32_cache = (uint32_t *)s_spi_frame_cache;
        for (i = 0; i < width; i++) {
            for (j = 0; j < length; j++) {
                p32_cache[index + j] = color;
            }
            index += device->width;
            row_num++;
        }
    }

    setCursor(device, 0, y);
    writeRegister(&sc_cmd_ramwr, NULL, 0);
    waitTransferDone(DEFAULT_TIMEOUT);
#if ST7796U_SPI_HARD_CS == 0
    s_spi_sequence_end = 1;
#endif
    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_HIGH);
    R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_LOW);
    if (device->color_format == COLOR_FORMAT_RGB565) {
        R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, &p16_cache[y * device->width], row_num * device->width * 2, SPI_BIT_WIDTH_8_BITS);
    }
    else {
        R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, &p32_cache[y * device->width], row_num * device->width * 4, SPI_BIT_WIDTH_8_BITS);
    }

#else
	uint8_t wlen;
	uint8_t data_write[4];
	uint16_t i, j;

	uint16_t x_end = x + length;
	uint16_t y_end = y + width;

	if (device->color_format == COLOR_FORMAT_RGB565) {
		wlen = 2;
		data_write[0] = (uint8_t)(color >> 8);
		data_write[1] = (uint8_t)color;
	}
	else {
		wlen = 4;
		data_write[0] = (uint8_t)(color >> 24);
		data_write[1] = (uint8_t)(color >> 16);
		data_write[2] = (uint8_t)(color >> 8);
		data_write[3] = (uint8_t)color;
	}
    setRenderArea(device, x, y, length, width);
    writeRegister(&sc_cmd_ramwr, NULL, 0);
    waitTransferDone(DEFAULT_TIMEOUT);
    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_HIGH);
    R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_LOW);
	for (i = x; i < x_end; i++) {
		for (j = y; j < y_end; j++) {
			R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, data_write, 2, SPI_BIT_WIDTH_8_BITS);
            waitTransferDone(DEFAULT_TIMEOUT);
		}
	}
    R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_HIGH);
#endif

#endif

	LOG_D(__FUNCTION__, "End");

	return 0;
}

uint32_t LCD_PortFillScreen(LCD_Device *device, uint32_t color)
{
	(void)color;

#if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI
#elif __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI
	int i;
    uint8_t data_write[4];
    uint16_t row_start, row_end;
    uint16_t column_start, column_end;

    if ((device->orientation == SCREEN_ORIENTATION_HORIZONTAL) || (device->orientation == SCREEN_ORIENTATION_HORIZONTAL_FLIP)) {
    	row_start = 0x31;
    	row_end = device->high + 0x31;
    	column_start = 0;
    	column_end = device->width;
    }
    else {
    	row_start = 0;
    	row_end = device->high;
    	column_start = 0x31;
    	column_end = device->width + 0x31;
    }

    LOG_D(__FUNCTION__, "row_start: %d", row_start);
    LOG_D(__FUNCTION__, "row_end: %d", row_end);
    LOG_D(__FUNCTION__, "column_start: %d", column_start);
    LOG_D(__FUNCTION__, "column_end: %d", column_end);

    waitTransferDone(DEFAULT_TIMEOUT);
    if (device->color_format == COLOR_FORMAT_RGB565) {
        data_write[0] = column_start >> 8;
        data_write[1] = (uint8_t)column_start;
        data_write[2] = (column_end - 1) >> 8;
        data_write[3] = (column_end - 1) & 0xFF;
        LOG_D(__FUNCTION__, "Set CASET");
        writeRegister(&sc_cmd_caset, data_write, 4);
        waitTransferDone(DEFAULT_TIMEOUT);
        data_write[0] = row_start >> 8;
        data_write[1] = (uint8_t)row_start;
        data_write[2] = (row_end - 1) >> 8;
        data_write[3] = (row_end - 1) & 0xFF;
        LOG_D(__FUNCTION__, "Set RASET");
        writeRegister(&sc_cmd_raset, data_write, 4);
        waitTransferDone(DEFAULT_TIMEOUT);
        writeRegister(&sc_cmd_ramwr, NULL, 0);
        waitTransferDone(DEFAULT_TIMEOUT);

        LOG_D(__FUNCTION__, "Fill Start");
    #if ST7796U_4SPI_EN_CACHE
        data_write[0] = (uint8_t)(color >> 8);
        data_write[1] = (uint8_t)color;
        uint16_t *p_color = (uint16_t *)data_write;
    #if ST7796U_4SPI_EN_DOUBLE_CACHE
        s_spi_frame_current = s_spi_frame_current ? 0 : 1;
        uint16_t *p_cache = (uint16_t *)&s_spi_frame_cache[s_spi_frame_current];
    #else
        uint16_t *p_cache = (uint16_t *)s_spi_frame_cache;
    #endif
        for (i = 0; i < ST7796U_CACHE_SIZE / 2; i++) {
            p_cache[i] = *p_color;
        }
	#if ST7796U_SPI_HARD_CS == 0
        s_spi_sequence_end = 1;
	#endif
        R_BSP_PinAccessEnable();
        R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_HIGH);
        R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_LOW);
        R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, p_cache, ST7796U_CACHE_SIZE, SPI_BIT_WIDTH_8_BITS);
    #else
	#if ST7796U_SPI_HARD_CS == 0
        s_spi_sequence_end = 0;
	#endif
        R_BSP_PinAccessEnable();
		R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_HIGH);
		R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_LOW);
        data_write[0] = (uint8_t)(color >> 8);
        data_write[1] = (uint8_t)color;
		for (i = 0; i < (device->high * device->width); i++) {
			R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, data_write, 2, SPI_BIT_WIDTH_8_BITS);
			waitTransferDone(DEFAULT_TIMEOUT);
		}
		R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_HIGH);
	#if ST7796U_SPI_HARD_CS == 0
		s_spi_sequence_end = 1;
	#endif
    #endif
    }

    LOG_D(__FUNCTION__, "Fill Done");
#endif

    return 0;
}

uint32_t LCD_PortInit(LCD_Device *device)
{
    ioport_instance_ctrl_t pin_ctrl;
    ioport_pin_cfg_t pin_cfg_data[6];
    ioport_cfg_t pin_cfg;
    int64_t time_start, time_end;
    size_t i, len_init_table;

#if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI
    fsp_err_t err;
    mipi_dsi_cmd_t msg;

    /* e2 doesn't generate this pin config */
    pin_cfg_data[0].pin = ST7796U_PIN_RESET;
    pin_cfg_data[0].pin_cfg = (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH;
    pin_cfg.number_of_pins = 1;
    pin_cfg.p_pin_cfg_data = (ioport_pin_cfg_t const *)&pin_cfg_data;
    pin_cfg.p_extend = NULL;
    R_IOPORT_Open(&pin_ctrl, &pin_cfg);

    hardReset();
    memset(fb_background, 0x10, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0);
    memset(&fb_background[1], 0x01, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0);
    R_GLCDC_Open(g_lcdc.p_ctrl, g_lcdc.p_cfg);
    msg.channel = 0;
    len_init_table = sizeof(sc_lcd_mipi_init_table) / sizeof(sc_lcd_mipi_init_table[0]);
    for (i = 0; i < len_init_table; i++) {
        msg.cmd_id = sc_lcd_mipi_init_table[i].cmd_id;
        msg.flags = sc_lcd_mipi_init_table[i].flags;
        msg.p_tx_buffer = sc_lcd_mipi_init_table[i].buffer;
        msg.tx_len = sc_lcd_mipi_init_table[i].size;
        s_mipi_tx_done = 0;
        err = R_MIPI_DSI_Command(g_mipi_dsi0.p_ctrl, &msg);
        if (err != FSP_SUCCESS) {
            LOG_E(__FUNCTION__, "MIPI_DSI_Command error: 0x%X", err);
        }
        while (s_mipi_tx_done == 0) {
        #ifdef __OPTIMIZE__
            __nop();
        #endif
        }
    }
    R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
    R_GLCDC_Start(g_lcdc.p_ctrl);
#elif __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI
    uint8_t data_read[8] = {0};

    /* LCD module reset pin */
    pin_cfg_data[0].pin = ST7796U_PIN_RESET;
    pin_cfg_data[0].pin_cfg = (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH;
    /* LCD module CSX pin, as SPI CS */
#if ST7796U_SPI_HARD_CS
    pin_cfg_data[1].pin = ST7796U_PIN_SPI_CS;
    pin_cfg_data[1].pin_cfg = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SCI0_2_4_6_8 | (uint32_t)IOPORT_CFG_DRIVE_HIGH;
#else
    pin_cfg_data[1].pin_cfg = (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH | (uint32_t)IOPORT_CFG_DRIVE_HIGH;
#endif
    /* LCD module D/CX pin */
    pin_cfg_data[2].pin = ST7796U_PIN_DCX;
    pin_cfg_data[2].pin_cfg = (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH | (uint32_t)IOPORT_CFG_DRIVE_HIGH;
    /* SCI4 Simple SPI: CLK */
    pin_cfg_data[3].pin = ST7796U_PIN_SPI_CLK;
    pin_cfg_data[3].pin_cfg = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SCI0_2_4_6_8 | (uint32_t)IOPORT_CFG_DRIVE_HIGH;
    /* SCI4 Simple SPI: MOSI */
    pin_cfg_data[4].pin = ST7796U_PIN_SPI_MOSI;
    pin_cfg_data[4].pin_cfg = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_SCI0_2_4_6_8 | (uint32_t)IOPORT_CFG_DRIVE_HIGH;
    /* SCI4 Simple SPI: MISO */
    pin_cfg_data[5].pin = ST7796U_PIN_SPI_MISO;
    pin_cfg_data[5].pin_cfg = (uint32_t)IOPORT_CFG_NMOS_ENABLE;
    pin_cfg.number_of_pins = 6;
    pin_cfg.p_pin_cfg_data = (ioport_pin_cfg_t const *)&pin_cfg_data;
    pin_cfg.p_extend = NULL;
    R_IOPORT_Open(&pin_ctrl, &pin_cfg);
    R_SCI_B_SPI_Open(ST7796U_SPI_INSTANCE.p_ctrl, ST7796U_SPI_INSTANCE.p_cfg);

    /* Init sequence */
    LOG_D(__FUNCTION__, "HardReset");
    hardReset();
    len_init_table = sizeof(sc_lcd_setting_table) / sizeof(sc_lcd_setting_table[0]);
    LOG_D(__FUNCTION__, "Send init squence");
    for (i = 0; i < len_init_table; i++) {
        if (sc_lcd_setting_table[i].cmd == NULL) {
            LOG_D(__FUNCTION__, "Delay %u ms", sc_lcd_setting_table[i].length);
            R_BSP_SoftwareDelay(sc_lcd_setting_table[i].length, BSP_DELAY_UNITS_MILLISECONDS);
        }
        else {
        #if LOG_CFG_LEVEL == LOG_LEVEL_DEBUG
            LOG_SetAutoEndl(0);
            LOG_D(__FUNCTION__, "Write cmd: 0x%02X", *sc_lcd_setting_table[i].cmd);
            if (sc_lcd_setting_table[i].length) {
                LOG_Puts(". val: ");
                for (int j = 0; j < sc_lcd_setting_table[i].length; j++) {
                    LOG_Printf("0x%02X ", sc_lcd_setting_table[i].val[j]);
                }
            }
            LOG_PutsEndl(NULL);
            LOG_SetAutoEndl(1);
        #endif
            writeRegister(sc_lcd_setting_table[i].cmd, sc_lcd_setting_table[i].val, sc_lcd_setting_table[i].length);
        }
    }

    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    readRegister(&sc_cmd_rddid, data_read, 4);
    waitTransferDone(DEFAULT_TIMEOUT);
    LOG_D(__FUNCTION__, "ID1: 0x%02X, ID2: 0x%02X, ID3: 0x%02X", data_read[1], data_read[2], data_read[3]);
    readRegister(&sc_cmd_rddst, data_read, 5);
#if LOG_CFG_LEVEL == LOG_LEVEL_DEBUG
    LOG_SetAutoEndl(0);
    LOG_D(__FUNCTION__, "Display status:");
    for (i = 1; i < 5; i++) {
    	LOG_Printf(" 0x%02X", data_read[i]);
    }
    LOG_PutsEndl(NULL);
    LOG_SetAutoEndl(1);
#endif

    const sci_b_spi_extended_cfg_t *p_extend = (sci_b_spi_extended_cfg_t *)ST7796U_SPI_INSTANCE.p_cfg->p_extend;
    s_spi_write_clk_config.clk_div = p_extend->clk_div;
    s_spi_write_clk_config.clock_source = p_extend->clock_source;

#if ST7796U_4SPI_EN_CACHE
#if ST7796U_4SPI_EN_DOUBLE_CACHE
    memset(s_spi_frame_cache, 0xFF, ST7796U_CACHE_SIZE * 2);
    s_spi_frame_current = 0;
#else
    memset(s_spi_frame_cache, 0xFF, ST7796U_CACHE_SIZE);
#endif
#endif

#else
#error "Unimplemented interface"
#endif /* #if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI */
    device->high = 480;
    device->width = 222;
    device->color_format = COLOR_FORMAT_RGB565;
    /* sc_lcd_setting_table sets screen oritation to vertical */
    if (device->orientation != SCREEN_ORIENTATION_VERTICAL) {
    	LCD_PortSetOrientation(device, device->orientation);
    }

    time_start = get_system_ms();
    LCD_PortFillScreen(device, 0xFFFF);
    waitTransferDone(DEFAULT_TIMEOUT * 2);
    time_end = get_system_ms();
    LOG_I(__FUNCTION__, "Fill screen spend %lld ms", time_end - time_start);
    /* Don't know why doesn't the above statement work. On RA8P1, it works */
    LCD_PortFillScreen(device, 0xFFFF);

    return 0;
}

uint32_t LCD_PortSetOrientation(LCD_Device *device, LCD_ScreenOrientation orientation)
{
#if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI
	(void)device;
	(void)orientation;
#elif __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI
	uint8_t data_write[4];

	LOG_D(__FUNCTION__, "Change orientation to %d", orientation);
	waitTransferDone(DEFAULT_TIMEOUT);
	if (orientation == SCREEN_ORIENTATION_HORIZONTAL) {
		data_write[0] = 0x28;
		writeRegister(&sc_cmd_madctl, data_write, 1);
		waitTransferDone(DEFAULT_TIMEOUT);
		data_write[0] = 0x00;
		data_write[1] = 0x00;
		data_write[2] = 0x01;
		data_write[3] = 0xDF;
		writeRegister(&sc_cmd_caset, data_write, 4);
		waitTransferDone(DEFAULT_TIMEOUT);
		data_write[2] = 0x01;
		data_write[3] = 0x0E;
		writeRegister(&sc_cmd_raset, data_write, 4);
		waitTransferDone(DEFAULT_TIMEOUT);
		device->high = 222;
		device->width = 480;
	}
	else if (orientation == SCREEN_ORIENTATION_HORIZONTAL_FLIP) {
		data_write[0] = 0xE8;
		writeRegister(&sc_cmd_madctl, data_write, 1);
		waitTransferDone(DEFAULT_TIMEOUT);
		data_write[0] = 0x00;
		data_write[1] = 0x00;
		data_write[2] = 0x01;
		data_write[3] = 0xDF;
		writeRegister(&sc_cmd_caset, data_write, 4);
		waitTransferDone(DEFAULT_TIMEOUT);
		data_write[2] = 0x01;
		data_write[3] = 0x0E;
		writeRegister(&sc_cmd_raset, data_write, 4);
		waitTransferDone(DEFAULT_TIMEOUT);
		device->high = 222;
		device->width = 480;
	}
	else if (orientation == SCREEN_ORIENTATION_VERTICAL) {
		data_write[0] = 0x48;
		writeRegister(&sc_cmd_madctl, data_write, 1);
		waitTransferDone(DEFAULT_TIMEOUT);
		data_write[0] = 0x00;
		data_write[1] = 0x00;
		data_write[2] = 0x01;
		data_write[3] = 0x0E;
		writeRegister(&sc_cmd_caset, data_write, 4);
		waitTransferDone(DEFAULT_TIMEOUT);
		data_write[2] = 0x01;
		data_write[3] = 0xDF;
		writeRegister(&sc_cmd_caset, data_write, 4);
		waitTransferDone(DEFAULT_TIMEOUT);
		device->high = 480;
		device->width = 222;
	}
	else {
		data_write[0] = 0x88;
		writeRegister(&sc_cmd_madctl, data_write, 1);
		waitTransferDone(DEFAULT_TIMEOUT);
		data_write[0] = 0x00;
		data_write[1] = 0x00;
		data_write[2] = 0x01;
		data_write[3] = 0x0E;
		writeRegister(&sc_cmd_caset, data_write, 4);
		waitTransferDone(DEFAULT_TIMEOUT);
		data_write[2] = 0x01;
		data_write[3] = 0xDF;
		writeRegister(&sc_cmd_caset, data_write, 4);
		waitTransferDone(DEFAULT_TIMEOUT);
		device->high = 480;
		device->width = 222;
	}
	device->orientation = orientation;
#endif

	return 0;
}

/*============================ LOCAL FUNCTIONS ===============================*/
static void hardReset(void)
{
    R_BSP_PinAccessEnable();
    R_IOPORT_PinWrite(&g_ioport_ctrl, ST7796U_PIN_RESET, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, ST7796U_PIN_RESET, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, ST7796U_PIN_RESET, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(4, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, ST7796U_PIN_RESET, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, ST7796U_PIN_RESET, BSP_IO_LEVEL_HIGH);
}

#if __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI
static void readMemory(uint8_t *val, uint32_t length)
{
    fsp_err_t err;

    /* Wait last transfer done */
    waitTransferDone(DEFAULT_TIMEOUT);

    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_LOW);
    R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_LOW);
    setClock(SPI_CLK_WRITE);
    s_spi_transfer_done = 0;
    err = R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, &sc_cmd_ramrd, 1, SPI_BIT_WIDTH_8_BITS);
    UNLIKE_RETURN(err, FSP_SUCCESS, "Write command failed: 0x%X", err);
    waitTransferDone(DEFAULT_TIMEOUT);
    R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_HIGH);
    /* Set flag, interrupt callback can driver CS to high when spi transfer complete */
#if ST7796U_SPI_HARD_CS == 0
    s_spi_sequence_end = 1;
#endif
    setClock(SPI_CLK_READ);
    s_spi_transfer_done = 0;
    err = R_SCI_B_SPI_Read(ST7796U_SPI_INSTANCE.p_ctrl, val, length, SPI_BIT_WIDTH_8_BITS);
    UNLIKE_RETURN(err, FSP_SUCCESS, "Read API failed: 0x%X", err);
}

static void readRegister(const uint8_t *reg, uint8_t *val, uint32_t length)
{
    fsp_err_t err;

    /* Wait last transfer done */
    waitTransferDone(DEFAULT_TIMEOUT);
#if ST7796U_SPI_HARD_CS == 0
    /* Clear flag, start a spi transfer squence */
    s_spi_sequence_end = 0;
#endif

    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_LOW);
    R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_LOW);
    setClock(SPI_CLK_WRITE);
    s_spi_transfer_done = 0;
    err = R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, reg, 1, SPI_BIT_WIDTH_8_BITS);
    UNLIKE_RETURN(err, FSP_SUCCESS, "Write command failed: 0x%X", err);
    waitTransferDone(DEFAULT_TIMEOUT);
    R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_HIGH);
#if ST7796U_SPI_HARD_CS == 0
    /* Set flag, interrupt callback can driver CS to high when spi transfer complete */
    s_spi_sequence_end = 1;
#endif
    setClock(SPI_CLK_READ);
    s_spi_transfer_done = 0;
    err = R_SCI_B_SPI_Read(ST7796U_SPI_INSTANCE.p_ctrl, val, length, SPI_BIT_WIDTH_8_BITS);
    UNLIKE_RETURN(err, FSP_SUCCESS, "Read API failed: 0x%X", err);
}

/**
 * @brief   Switch SPI clock. ST7796U has different maximum clock when reading and writing.
 * @param   select SPI_CLK_READ: Switch to slow clock. SPI_CLK_WRITE: Switch to fast clock.
 */
static void setClock(lcd_spi_clk_select_t select)
{
    const sci_b_spi_extended_cfg_t *p_cfg;
    R_SCI_B0_Type *p_reg = ((sci_b_spi_instance_ctrl_t *)ST7796U_SPI_INSTANCE.p_ctrl)->p_reg;

    if (select == SPI_CLK_READ) {
        p_cfg = &sc_spi_read_clk_config;
    }
    else {
        p_cfg = &s_spi_write_clk_config;
    }

    if ((p_reg->CCR2_b.CKS != p_cfg->clk_div.cks) ||
        (p_reg->CCR2_b.BRR != p_cfg->clk_div.brr) ||
        (p_reg->CCR2_b.BGDM != p_cfg->clk_div.bgdm) ||
        (p_reg->CCR3_b.BPEN != p_cfg->clock_source)) {
        waitTransferDone(DEFAULT_TIMEOUT);
        R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MICROSECONDS);
        p_reg->CCR2_b.CKS = p_cfg->clk_div.cks;
        p_reg->CCR2_b.BRR = p_cfg->clk_div.brr;
        p_reg->CCR2_b.BGDM = p_cfg->clk_div.bgdm;
        p_reg->CCR3_b.BPEN = p_cfg->clock_source;
        R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MICROSECONDS);
    }
}

static void setCursor(LCD_Device *device, uint16_t x, uint16_t y)
{
	uint8_t data_write[2];

    if ((device->orientation == SCREEN_ORIENTATION_HORIZONTAL) || (device->orientation == SCREEN_ORIENTATION_HORIZONTAL_FLIP)) {
        y += 0x31;
    }
    else {
        x += 0x31;
    }

	data_write[0] = x >> 8;
	data_write[1] = (uint8_t)x;
	writeRegister(&sc_cmd_caset, data_write, 2);
	waitTransferDone(DEFAULT_TIMEOUT);
	data_write[0] = y >> 8;
	data_write[1] = (uint8_t)y;
	writeRegister(&sc_cmd_raset, data_write, 2);
	waitTransferDone(DEFAULT_TIMEOUT);
}

static void setRenderArea(LCD_Device *device, uint16_t x, uint16_t y, uint16_t length, uint16_t width)
{
	uint8_t data_write[4];

	if ((device->orientation == SCREEN_ORIENTATION_HORIZONTAL) || (device->orientation == SCREEN_ORIENTATION_HORIZONTAL_FLIP)) {
        y += 0x31;
    }
    else {
        x += 0x31;
    }

	data_write[0] = x >> 8;
	data_write[1] = (uint8_t)x;
	data_write[2] = (x + length - 1) >> 8;
	data_write[3] = (uint8_t)(x + length - 1);
	writeRegister(&sc_cmd_caset, data_write, 4);
	waitTransferDone(DEFAULT_TIMEOUT);
	data_write[0] = y >> 8;
	data_write[1] = (uint8_t)y;
	data_write[2] = (y + width - 1) >> 8;
	data_write[3] = (uint8_t)(y + width - 1);
	writeRegister(&sc_cmd_raset, data_write, 4);
	waitTransferDone(DEFAULT_TIMEOUT);
}

static void waitTransferDone(uint32_t timeout_ms)
{
    volatile uint32_t us = timeout_ms * 1000;
    sci_b_spi_instance_ctrl_t *p_ctrl = ST7796U_SPI_INSTANCE.p_ctrl;

    /* This seems does't work correctly */
    while (us) {
    	if (s_spi_transfer_done == 0) {
    		R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
    		us--;
    	}
    	else {
    		us = 0;
    	}
    }

    while (us) {
        if ((p_ctrl->p_reg->CCR0 & (R_SCI_B0_CCR0_RE_Msk | R_SCI_B0_CCR0_TE_Msk)) != 0) {
            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
            us--;
        }
        else {
            us = 0;
        }
    }
}

static void writeMemory(const uint8_t *val, uint32_t length)
{
    fsp_err_t err;

    /* Wait last transfer done */
    waitTransferDone(DEFAULT_TIMEOUT);
#if ST7796U_SPI_HARD_CS == 0
    /* Clear flag, start a spi transfer squence */
    s_spi_sequence_end = 0;
#endif

    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_LOW);
    R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_LOW);
    setClock(SPI_CLK_WRITE);
    s_spi_transfer_done = 0;
    err = R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, &sc_cmd_ramwr, 1, SPI_BIT_WIDTH_8_BITS);
    UNLIKE_RETURN(err, FSP_SUCCESS, "Write command failed: 0x%X", err);
    waitTransferDone(DEFAULT_TIMEOUT);
    R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_HIGH);
#if ST7796U_SPI_HARD_CS == 0
    /* Set flag, interrupt callback can driver CS to high when spi transfer complete */
    s_spi_sequence_end = 1;
#endif
    setClock(SPI_CLK_WRITE);
    s_spi_transfer_done = 0;
    err = R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, val, length, SPI_BIT_WIDTH_8_BITS);
    UNLIKE_RETURN(err, FSP_SUCCESS, "Read API failed: 0x%X", err);
}

static void writeRegister(const uint8_t *reg, const uint8_t *val, uint32_t length)
{
    fsp_err_t err;

    /* Wait last transfer done */
    waitTransferDone(DEFAULT_TIMEOUT);
#if ST7796U_SPI_HARD_CS == 0
    /* Clear flag, start a spi transfer squence */
    s_spi_sequence_end = 0;
#endif

    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_LOW);
    R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_LOW);
    setClock(SPI_CLK_WRITE);
    s_spi_transfer_done = 0;
    err = R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, reg, 1, SPI_BIT_WIDTH_8_BITS);
    UNLIKE_RETURN(err, FSP_SUCCESS, "Write command failed: 0x%X", err);
    waitTransferDone(DEFAULT_TIMEOUT);
#if ST7796U_SPI_HARD_CS == 0
    /* Set flag, interrupt callback can driver CS to high when spi transfer complete */
    s_spi_sequence_end = 1;
#endif
    if ((val != NULL) && (length != 0)) {
        R_BSP_PinWrite(ST7796U_PIN_DCX, BSP_IO_LEVEL_HIGH);
        setClock(SPI_CLK_WRITE);
        s_spi_transfer_done = 0;
        err = R_SCI_B_SPI_Write(ST7796U_SPI_INSTANCE.p_ctrl, val, length, SPI_BIT_WIDTH_8_BITS);
        UNLIKE_RETURN(err, FSP_SUCCESS, "Read API failed: 0x%X", err);
    }
    else {
        R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_HIGH);
    }
}
#endif /* #if __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI */

/*============================ IMPLEMENTATION ================================*/
#if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI
void GLCDC_Callback(display_callback_args_t *p_args)
{
    if (p_args->event == DISPLAY_EVENT_LINE_DETECTION) {
        s_glcdc_vsync_flag = 1;
    }
}

void MIPI_DSI_Callback(mipi_dsi_callback_args_t *p_args)
{
    if (p_args->event == MIPI_DSI_EVENT_SEQUENCE_0) {
        if (p_args->tx_status == MIPI_DSI_SEQUENCE_STATUS_DESCRIPTORS_FINISHED) {
            s_mipi_tx_done = 1;
        }
    }
}

#elif __ST7796U_INTERFACE == ST7796U_INTERFACE_4SPI
void ST7796U_SPI_CALLBACK(spi_callback_args_t *p_args)
{
    if (p_args->event == SPI_EVENT_TRANSFER_COMPLETE) {
    	s_spi_transfer_done = 1;
    #if ST7796U_SPI_HARD_CS == 0
        if (s_spi_sequence_end) {
            R_BSP_PinWrite(ST7796U_PIN_SPI_CS, BSP_IO_LEVEL_HIGH);
        }
    #endif
    }
    else {
        if (p_args->event == SPI_EVENT_TRANSFER_ABORTED) {
            LOG_W(__FUNCTION__, "Abort");
        }
        else {
            LOG_E(__FUNCTION__, "Event: %d", p_args->event);
        }
    }
}
#endif /* #if __ST7796U_INTERFACE == ST7796U_INTERFACE_MIPI */
