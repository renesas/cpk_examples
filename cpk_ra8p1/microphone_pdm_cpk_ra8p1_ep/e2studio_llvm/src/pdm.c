#include "hal_data.h"
#include "pdm.h"

#define PDM_INSTANCE_L	g_pdm1_left
#define PDM_INSTANCE_R	g_pdm1_right
#define PDM_CALLBACK_L	PDM_LEFT_Callback
#define PDM_CALLBACK_R	PDM_RIGHT_Callback

#define TAG __FUNCTION__

#ifndef __PDM_DEBUG
#define __PDM_DEBUG	1
#endif

#if __PDM_DEBUG
#include "utils/log.h"
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(TAG, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(TAG, msg, ##__VA_ARGS__); return v; }
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#endif

static PDM_CallbackFunc s_callback_funcs[PDM_ON_ERROR + 1];

uint32_t PDM_Init(void)
{
	uint32_t err;
	uint32_t i;
	ioport_instance_ctrl_t pin_ctrl;
	ioport_pin_cfg_t pin_cfg_data[2];
    ioport_cfg_t pin_cfg;
    pdm_sound_detection_setting_t sound_detection_setting;

	for (i = 0; i < (PDM_ON_ERROR + 1); i++) {
		s_callback_funcs[i] = NULL;
	}

    pin_cfg_data[0].pin = BSP_IO_PORT_05_PIN_01;
    pin_cfg_data[0].pin_cfg = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_PDM;
    pin_cfg_data[1].pin = BSP_IO_PORT_08_PIN_11;
    pin_cfg_data[1].pin_cfg = (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_PDM;

    pin_cfg.number_of_pins = 2;
    pin_cfg.p_pin_cfg_data = (ioport_pin_cfg_t const *)&pin_cfg_data;
    pin_cfg.p_extend = NULL;
    R_IOPORT_Open(&pin_ctrl, &pin_cfg);

    err = R_PDM_Open(PDM_INSTANCE_L.p_ctrl, PDM_INSTANCE_L.p_cfg);
    UNLIKE_RETURN(err, 0, "Left Open failed: %u", err);
    err = R_PDM_Open(PDM_INSTANCE_R.p_ctrl, PDM_INSTANCE_R.p_cfg);
    UNLIKE_RETURN(err, 0, "Right Open failed: %u", err);
    R_BSP_SoftwareDelay(PDM1_FILTER_SETTLING_TIME_US + 35000, BSP_DELAY_UNITS_MICROSECONDS);

    sound_detection_setting.sound_detection_lower_limit = 0xFFF80000;
    sound_detection_setting.sound_detection_upper_limit = 5000;
    err = R_PDM_SoundDetectionEnable(PDM_INSTANCE_L.p_ctrl, sound_detection_setting);
    UNLIKE_RETURN(err, 0, "Enable failed: %u", err);

	return 0;
}

PDM_CallbackFunc PDM_RegisterCb(PDM_CallbackFunc cb, PDM_CallbackEnum cbe)
{
	PDM_CallbackFunc old;

	if (cbe > PDM_ON_ERROR) {
		return NULL;
	}

	old = s_callback_funcs[cbe];
	s_callback_funcs[cbe] = cb;

	return old;
}

uint32_t PDM_Start(void *ldata, void *rdata, uint32_t length, uint32_t num_to_callback)
{
	uint32_t err;
	uint32_t stop_err;

	err = R_PDM_Start(PDM_INSTANCE_L.p_ctrl, ldata, length, num_to_callback);
	UNLIKE_RETURN(err, 0, "Left Start failed: %u", err);
	err = R_PDM_Start(PDM_INSTANCE_R.p_ctrl, rdata, length, num_to_callback);
	if (err != 0) {
		LOG_E(TAG, "Right Start failed: %u", err);
		stop_err = R_PDM_Stop(PDM_INSTANCE_L.p_ctrl);
		if (stop_err != 0) {
			LOG_E(TAG, "Left rollback Stop failed: %u", stop_err);
		}
		return err;
	}

	return 0;
}

uint32_t PDM_Stop(void)
{
	uint32_t left_err;
	uint32_t right_err;

	left_err = R_PDM_Stop(PDM_INSTANCE_L.p_ctrl);
	right_err = R_PDM_Stop(PDM_INSTANCE_R.p_ctrl);
	if (left_err != 0) {
		LOG_E(TAG, "Left Stop failed: %u", left_err);
		return left_err;
	}
	if (right_err != 0) {
		LOG_E(TAG, "Right Stop failed: %u", right_err);
		return right_err;
	}

	return 0;
}

void PDM_CALLBACK_L(pdm_callback_args_t *p_args)
{
	switch (p_args->event) {
	case PDM_EVENT_IDLE:
		break;
	case PDM_EVENT_DATA:
	#if __PDM_DEBUG
		LOG_D(TAG, "EVENT_DATA");
	#endif
		if (s_callback_funcs[PDM_ON_LEFT_DATA] != NULL) {
			s_callback_funcs[PDM_ON_LEFT_DATA]();
		}
		break;
	case PDM_EVENT_SOUND_DETECTION:
	#if __PDM_DEBUG
		LOG_D(TAG, "EVENT_SOUND_DETECTION");
	#endif
		break;
	case PDM_EVENT_ERROR:
	#if __PDM_DEBUG
		LOG_E(TAG, "EVENT_ERROR: 0x%X", p_args->error);
	#endif
		switch (p_args->error) {
		case PDM_ERROR_NONE:
			break;
		case PDM_ERROR_SHORT_CIRCUIT:
			break;
		case PDM_ERROR_OVERVOLTAGE_LOWER:
			break;
		case PDM_ERROR_OVERVOLTAGE_UPPER:
			break;
		case PDM_ERROR_BUFFER_OVERWRITE:
			break;
		default:
			break;
		}
		if (s_callback_funcs[PDM_ON_ERROR] != NULL) {
			s_callback_funcs[PDM_ON_ERROR]();
		}
		break;
	default:
		break;
	}
}

void PDM_CALLBACK_R(pdm_callback_args_t *p_args)
{
	switch (p_args->event) {
	case PDM_EVENT_IDLE:
		break;
	case PDM_EVENT_DATA:
	#if __PDM_DEBUG
		LOG_D(TAG, "EVENT_DATA");
	#endif
		if (s_callback_funcs[PDM_ON_RIGHT_DATA] != NULL) {
			s_callback_funcs[PDM_ON_RIGHT_DATA]();
		}
		break;
	case PDM_EVENT_SOUND_DETECTION:
	#if __PDM_DEBUG
		LOG_D(TAG, "EVENT_SOUND_DETECTION");
	#endif
		break;
	case PDM_EVENT_ERROR:
	#if __PDM_DEBUG
		LOG_E(TAG, "EVENT_ERROR: 0x%X", p_args->error);
	#endif
		switch (p_args->error) {
		case PDM_ERROR_NONE:
			break;
		case PDM_ERROR_SHORT_CIRCUIT:
			break;
		case PDM_ERROR_OVERVOLTAGE_LOWER:
			break;
		case PDM_ERROR_OVERVOLTAGE_UPPER:
			break;
		case PDM_ERROR_BUFFER_OVERWRITE:
			break;
		default:
			break;
		}
		if (s_callback_funcs[PDM_ON_ERROR] != NULL) {
			s_callback_funcs[PDM_ON_ERROR]();
		}
		break;
	default:
		break;
	}
}
