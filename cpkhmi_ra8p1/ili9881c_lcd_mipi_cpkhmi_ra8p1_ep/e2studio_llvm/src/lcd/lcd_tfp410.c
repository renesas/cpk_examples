#include "lcd.h"

#if LCD_DIRVER == LCD_DRIVER_TFP410

#include "hal_data.h"
#include "iic.h"

#define TFP410_IIC_ADDRESS		0x38
#define TFP410_GLCDC_INSTANCE	g_lcdc_tfp410
#define TFP410_GLCDC_CALLBACK	GLCDC_Callback

#ifndef __TFP410_DEBUG
#define __TFP410_DEBUG	1
#endif

#define TFP410_VEN_ID		0x014C
#define TFP410_DEV_ID		0x0410

/* TFP410 register subaddress */
#define TFP410_VEN_ID_L		0x00
#define TFP410_VEN_ID_H		0x01
#define TFP410_DEV_ID_L		0x02
#define TFP410_DEV_ID_H		0x03
#define TFP410_CTL1_MODE	0x08
#define TFP410_CTL2_MODE	0x09
#define TFP410_CTL3_MODE	0x0A
#define TFP410_CFG			0x0B
#define TFP410_DE_DLY		0x32
#define TFP410_DE_CTL		0x33
#define TFP410_DE_TOP		0x34
#define TFP410_DE_CNT_L		0x36
#define TFP410_DE_CNT_H		0x37
#define TFP410_DE_LIN_L		0x38
#define TFP410_DE_LIN_H		0x39
#define TFP410_H_RES_L		0x3A
#define TFP410_H_RES_H		0x3B
#define TFP410_V_RES_L		0x3C
#define TFP410_V_RES_H		0x3D

#if __TFP410_DEBUG
#include "utils/log.h"
#endif

uint32_t LCD_PortFillRectangle(LCD_Device *device, uint16_t x, uint16_t y, uint16_t length, uint16_t width, uint32_t color)
{
	uint16_t i, j;

	glcdc_instance_ctrl_t *pctrl = (glcdc_instance_ctrl_t *)TFP410_GLCDC_INSTANCE.p_ctrl;
	uint32_t *p32_bg = pctrl->p_cfg->input[0].p_base;

	if ((x >= device->length) || (y >= device->width)) {
		return FSP_ERR_ASSERTION;
	}

	if ((x + length) > device->length) {
		length = device->length - x;
	}
	if ((y + width) > device->width) {
		width = device->width - y;
	}

	p32_bg = &p32_bg[y * device->length];
	for (i = 0; i < width; i++) {
		for (j = x; j < (x + length); j++) {
			p32_bg[j] = color;
		}
		p32_bg = &p32_bg[device->length];
	}

	return FSP_SUCCESS;
}

uint32_t LCD_PortInit(LCD_Device *device)
{
	uint8_t data_r[4];
    uint8_t data_w[4];

    uint16_t *p16 = NULL;

    data_w[0] = TFP410_DEV_ID_L;
    IIC_WriteRead(TFP410_IIC_ADDRESS, data_w, 1, data_r, 2);
    p16 = (uint16_t *)data_r;
    if (*p16 != TFP410_DEV_ID) {
	#if __TFP410_DEBUG
    	LOG_E(__FUNCTION__, "Check DEV_ID failed, expec: 0x%X, read: 0x%X", TFP410_DEV_ID, *p16);
	#endif
    	return FSP_ERR_UNSUPPORTED;
    }

    data_w[0] = TFP410_CTL1_MODE;
    data_w[1] = 0xBF;
    data_w[2] = 0x70;
    IIC_Write(TFP410_IIC_ADDRESS, data_w, 3);
    data_w[0] = TFP410_CTL3_MODE;
    data_w[1] = 0x90;
    IIC_Write(TFP410_IIC_ADDRESS, data_w, 2);

    R_GLCDC_Open(TFP410_GLCDC_INSTANCE.p_ctrl, TFP410_GLCDC_INSTANCE.p_cfg);
    R_GLCDC_Start(TFP410_GLCDC_INSTANCE.p_ctrl);

    device->color_format = COLOR_FORMAT_RGB888;
    device->length = DISPLAY_HSIZE_INPUT0;
    device->length_dummy = 0;
    device->orientation = SCREEN_ORIENTATION_HORIZONTAL;
    device->width = DISPLAY_VSIZE_INPUT0;
    device->width_dummy = 0;

	return 0;
}

void TFP410_GLCDC_CALLBACK(display_callback_args_t *p_args)
{
	(void)p_args;
}

#endif
