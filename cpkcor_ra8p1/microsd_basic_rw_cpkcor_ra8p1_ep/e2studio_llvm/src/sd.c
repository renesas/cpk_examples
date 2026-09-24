#include <inttypes.h>
#include "hal_data.h"
#include "sd.h"
#include "utils/util.h"

#if BSP_CFG_RTOS == 2
#include "FreeRTOS.h"
#include "event_groups.h"
#endif

#define SD_INSTANCE		g_rm_block_media0
#define SD_INSTANCE_CFG	UTIL_CONCAT(SD_INSTANCE, _cfg)
#define SD_CALLBACK		RM_BLOCK_MEDIA_Callback
#define SD_CD_PIN		BSP_IO_PORT_13_PIN_07

#ifndef __SD_DEBUG
#define __SD_DEBUG	1
#endif

#if __SD_DEBUG
#include "utils/log.h"
#define TAG	__FUNCTION__
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(TAG, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(TAG, msg, ##__VA_ARGS__); return v; }
#define SD_LOGD(msg, ...)				LOG_D(__FUNCTION__, msg, ##__VA_ARGS__)
#define SD_LOGI(msg, ...)				LOG_I(__FUNCTION__, msg, ##__VA_ARGS__)
#define SD_LOGW(msg, ...)				LOG_W(__FUNCTION__, msg, ##__VA_ARGS__)
#define SD_LOGE(msg, ...)				LOG_E(__FUNCTION__, msg, ##__VA_ARGS__)
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#define SD_LOGD(msg, ...)
#define SD_LOGW(msg, ...)
#define SD_LOGE(msg, ...)
#endif

/* FSP 的回调函数可能同时给出卡插入和卡移除事件，使用这个标志记录这种情况
 * 当发生这种情况时，SD_IsInsert() 会同时读取 CD 引脚的电平来共同判断是插入还是移除 */
static bool s_insert_remove_exist;
static bool s_use_cd_pin;
static volatile bool s_inserted;
static volatile uint8_t s_trans_done;

uint32_t SD_Init(void)
{
	uint32_t err;

	const rm_block_media_sdmmc_extended_cfg_t *p_extend_cfg = (const rm_block_media_sdmmc_extended_cfg_t *)SD_INSTANCE_CFG.p_extend;
	sdhi_instance_ctrl_t *p_ctrl = (sdhi_instance_ctrl_t *)p_extend_cfg->p_sdmmc->p_ctrl;
	sdmmc_cfg_t *p_cfg = (sdmmc_cfg_t *)p_extend_cfg->p_sdmmc->p_cfg;

	err = RM_BLOCK_MEDIA_SDMMC_Open(SD_INSTANCE.p_ctrl, SD_INSTANCE.p_cfg);
	UNLIKE_RETURN(err, 0, "Open failed: %u", err);
	SD_LOGD("SD_OPION: 0x%08" PRIX32, p_ctrl->p_reg->SD_OPTION);
	s_insert_remove_exist = false;
	if (p_cfg->card_detect == SDMMC_CARD_DETECT_CD) {
		s_use_cd_pin = true;
	}
	else {
		s_use_cd_pin = false;
	}

	return 0;
}

uint32_t SD_InitMedia(void)
{
	uint32_t err;
	rm_block_media_status_t status;

	RM_BLOCK_MEDIA_SDMMC_StatusGet(SD_INSTANCE.p_ctrl, &status);
	if (status.media_inserted != true) {
		SD_LOGE("SD Card not inserted");
		return FSP_ERR_ASSERTION;
	}
	R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
	err = RM_BLOCK_MEDIA_SDMMC_MediaInit(SD_INSTANCE.p_ctrl);
	UNLIKE_RETURN(err, 0, "MediaInit failed: %" PRIu32, err);
	s_trans_done = 1;

	return 0;
}

uint32_t SD_IsInsertRemoveExist(void)
{
	return s_insert_remove_exist ? 1 : 0;
}

uint32_t SD_IsInsert(void)
{
	bsp_io_level_t level;
	rm_block_media_status_t status;

	const rm_block_media_sdmmc_extended_cfg_t *p_extend_cfg = (const rm_block_media_sdmmc_extended_cfg_t *)SD_INSTANCE_CFG.p_extend;
	sdmmc_cfg_t *p_sdmmc_cfg = (sdmmc_cfg_t *)p_extend_cfg->p_sdmmc->p_cfg;

	if (p_sdmmc_cfg->card_detect == SDMMC_CARD_DETECT_CD) {
		RM_BLOCK_MEDIA_SDMMC_StatusGet(SD_INSTANCE.p_ctrl, &status);
		if (s_insert_remove_exist) {
			R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
			R_IOPORT_PinRead(g_ioport.p_ctrl, SD_CD_PIN, &level);
			if (level == BSP_IO_LEVEL_HIGH) {
				s_inserted = false;
			}
			else {
				s_inserted = true;
			}
			s_insert_remove_exist = false;
		}
		if (status.media_inserted && s_inserted) {
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		/* 在不使用 CD 功能时，RM_BLOCK_MEDIA_SDMMC_StatusGet() 会直接设置 media_inserted 为 true
		 * 如果要实现自己的判断逻辑，在这里修改 */
		return 1;
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
		SD_LOGW("Receive REMOVED and INSERTED simultaneously");
		p_args->event &= ~RM_BLOCK_MEDIA_EVENT_MEDIA_REMOVED;
		p_args->event &= ~RM_BLOCK_MEDIA_EVENT_MEDIA_INSERTED;
		s_insert_remove_exist = true;
	}

	if (p_args->event & RM_BLOCK_MEDIA_EVENT_MEDIA_REMOVED) {
		SD_LOGD("Removed");
		s_inserted = false;
		s_insert_remove_exist = false;
	}
	if (p_args->event & RM_BLOCK_MEDIA_EVENT_MEDIA_INSERTED) {
		SD_LOGD("Inserted");
		s_inserted = true;
		s_insert_remove_exist = false;
	}
	if (p_args->event & RM_BLOCK_MEDIA_EVENT_OPERATION_COMPLETE) {
		s_trans_done = 1;
	}
	if (p_args->event & RM_BLOCK_MEDIA_EVENT_ERROR) {
		SD_LOGE("Error");
	}
	if (p_args->event & RM_BLOCK_MEDIA_EVENT_POLL_STATUS) {}
	if (p_args->event & RM_BLOCK_MEDIA_EVENT_MEDIA_SUSPEND) {
		SD_LOGD("Suspend");
	}
	if (p_args->event & RM_BLOCK_MEDIA_EVENT_MEDIA_RESUME) {
		SD_LOGD("Resume");
	}
	if (p_args->event & RM_BLOCK_MEDIA_EVENT_WAIT) {}
	if (p_args->event & RM_BLOCK_MEDIA_EVENT_WAIT_END) {}
}
