#include "hal_data.h"
#include "sd.h"

#define SD_INSTANCE		g_rm_block_media0
#define SD_CALLBACK		RM_BLOCK_MEDIA_Callback

#ifndef __SD_DEBUG
#define __SD_DEBUG	1
#endif

#if __SD_DEBUG
#include "utils/log.h"
#define TAG	__FUNCTION__
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(TAG, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(TAG, msg, ##__VA_ARGS__); return v; }
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#endif

static volatile uint8_t s_inserted;
static volatile uint8_t s_trans_done;

uint32_t SD_Init(void)
{
	uint32_t err;

	err = RM_BLOCK_MEDIA_SDMMC_Open(SD_INSTANCE.p_ctrl, SD_INSTANCE.p_cfg);
	UNLIKE_RETURN(err, 0, "Open failed: %u", err);

	return 0;
}

uint32_t SD_InitMedia(void)
{
	uint32_t err;
	rm_block_media_status_t status;

	RM_BLOCK_MEDIA_SDMMC_StatusGet(SD_INSTANCE.p_ctrl, &status);
	if (status.media_inserted != true) {
		while (s_inserted == 0) {
			R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
		}
	#if __SD_DEBUG
		LOG_D(TAG, "Detect SD Card insert");
	#endif
	}
	R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
	err = RM_BLOCK_MEDIA_SDMMC_MediaInit(SD_INSTANCE.p_ctrl);
	UNLIKE_RETURN(err, 0, "MediaInit failed: %u", err);
	s_trans_done = 1;

	return 0;
}

uint32_t SD_IsInsert(void)
{
	rm_block_media_status_t status;

	RM_BLOCK_MEDIA_SDMMC_StatusGet(SD_INSTANCE.p_ctrl, &status);
	if (status.media_inserted && s_inserted) {
		return 1;
	}
	else {
		return 0;
	}
}

uint32_t SD_IsTransDone(void)
{
	return s_trans_done;
}

uint32_t SD_Read(uint8_t *data, uint32_t block_addr, uint32_t size)
{
	uint32_t i;
	uint32_t err;
	uint32_t num_blocks;
	uint32_t repeat;

	uint8_t *p_read = data;

	num_blocks = size / 512;
	repeat = num_blocks / 0x10000;

	for (i = 0; i < repeat; i++) {
		err = RM_BLOCK_MEDIA_SDMMC_Read(SD_INSTANCE.p_ctrl, p_read, block_addr, 0x10000);
		UNLIKE_RETURN(err, 0, "Read failed: %u", err);
		s_trans_done = 0;
		while (s_trans_done == 0) {
			R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
		}
		block_addr += 0x10000;
		num_blocks -= 0x10000;
		p_read = &p_read[512 * 0x10000];
	}

	err = RM_BLOCK_MEDIA_SDMMC_Read(SD_INSTANCE.p_ctrl, p_read, block_addr, num_blocks);
	UNLIKE_RETURN(err, 0, "Read failed: %u", err);
	s_trans_done = 0;

	return 0;
}

uint32_t SD_WaitTrans(void)
{
	rm_block_media_status_t status;

	RM_BLOCK_MEDIA_SDMMC_StatusGet(SD_INSTANCE.p_ctrl, &status);
	while (status.busy == true) {
		R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
		RM_BLOCK_MEDIA_SDMMC_StatusGet(SD_INSTANCE.p_ctrl, &status);
	}

	while (s_trans_done == 0) {
		R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
	}

	return 0;
}

uint32_t SD_Write(uint8_t const *src, uint32_t block_addr, uint32_t size)
{
	uint32_t i;
	uint32_t err;
	uint32_t num_blocks;
	uint32_t repeat;

	uint8_t const *p8 = src;

	if (s_trans_done == 0) {
		return FSP_ERR_IN_USE;
	}

	num_blocks = size / 512;
	repeat = num_blocks / 0x10000;

	for (i = 0; i < repeat; i++) {
		err = RM_BLOCK_MEDIA_SDMMC_Write(SD_INSTANCE.p_ctrl, p8, block_addr, 0x10000);
		UNLIKE_RETURN(err, 0, "Write failed: %u", err);
		s_trans_done = 0;
		while (s_trans_done == 0) {
			R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
		}
		block_addr += 0x10000;
		num_blocks -= 0x10000;
		p8 = &p8[512 * 0x10000];
	}

	err = RM_BLOCK_MEDIA_SDMMC_Write(SD_INSTANCE.p_ctrl, p8, block_addr, num_blocks);
	UNLIKE_RETURN(err, 0, "Write failed: %u", err);
	s_trans_done = 0;

	return 0;
}

void SD_CALLBACK(rm_block_media_callback_args_t *p_args)
{
	if (p_args->event == (RM_BLOCK_MEDIA_EVENT_MEDIA_REMOVED | RM_BLOCK_MEDIA_EVENT_MEDIA_INSERTED)) {
		p_args->event &= (~RM_BLOCK_MEDIA_EVENT_MEDIA_INSERTED);
	}

	switch (p_args->event) {
	case RM_BLOCK_MEDIA_EVENT_MEDIA_REMOVED:
	#if __SD_DEBUG
		LOG_D(TAG, "Removed");
	#endif
		s_inserted = 0;
		break;
	case RM_BLOCK_MEDIA_EVENT_MEDIA_INSERTED:
	#if __SD_DEBUG
		LOG_D(TAG, "Inserted");
	#endif
		s_inserted = 1;
		break;
	case RM_BLOCK_MEDIA_EVENT_OPERATION_COMPLETE:
		s_trans_done = 1;
		break;
	case RM_BLOCK_MEDIA_EVENT_ERROR:
		break;
	case RM_BLOCK_MEDIA_EVENT_POLL_STATUS:
		break;
	case RM_BLOCK_MEDIA_EVENT_MEDIA_SUSPEND:
	#if __SD_DEBUG
		LOG_D(TAG, "Suspend");
	#endif
		break;
	case RM_BLOCK_MEDIA_EVENT_MEDIA_RESUME:
	#if __SD_DEBUG
		LOG_D(TAG, "Resume");
	#endif
		break;
	case RM_BLOCK_MEDIA_EVENT_WAIT:
		break;
	case RM_BLOCK_MEDIA_EVENT_WAIT_END:
		break;
	default:
		break;
	}
}
