#include "test.h"

#if TEST_EN_AUDIO

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "console.h"
#include "es8311.h"
#include "hal_data.h"
#include "utils/log.h"

#define SSI_CHUNK_SIZE	1024

#ifndef TEST_AUDIO_EN_SIN_WAVE
#define TEST_AUDIO_EN_SIN_WAVE	0
#endif

#ifndef TEST_AUDIO_EN_MUSIC
#define TEST_AUDIO_EN_MUSIC		0
#endif

#ifndef TEST_AUDIO_EN_MIC
#define TEST_AUDIO_EN_MIC		1
#endif

#if TEST_AUDIO_EN_SIN_WAVE || TEST_AUDIO_EN_MUSIC
static int16_t s_tx_buffer[SSI_CHUNK_SIZE];
#endif

#if TEST_AUDIO_EN_MUSIC
#include "pcm_data.h"
static uint32_t s_play_pos = 0;
#endif

#if TEST_AUDIO_EN_MIC
#define MIC_RECORD_SECS         5
#define MIC_RECORD_SAMPLES      (44100 * MIC_RECORD_SECS * 2)
#define MIC_SAMPLES_PER_CHUNK   (SSI_CHUNK_SIZE / 2)

static int16_t s_record_buf[MIC_RECORD_SAMPLES];
static uint8_t s_has_record;
static uint8_t s_recording;
static uint8_t s_play_done;
#endif

static void onIdle(void);

uint32_t TestAudio(void)
{
#if TEST_AUDIO_EN_SIN_WAVE
	static float t = 0.0f;

	float input;
	int16_t sample;
	uint32_t i;

	uint32_t pclkd_get_hz = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKD);
	uint32_t period = pclkd_get_hz / g_timer2_cfg.period_counts;
	uint32_t audio_sample = (period / (2 * 16));  /* BCLK / (2ch * 16bit) */

	LOG_D(__FUNCTION__, "pclkd_get_hz: %u", pclkd_get_hz);
	LOG_D(__FUNCTION__, "period: %u", period);
	LOG_D(__FUNCTION__, "audio_sample: %u", audio_sample);

	for (i = 0; i < SSI_CHUNK_SIZE / 2; i++) {
		input = 2.0f * 3.14159f * 8000.0f * t / (float)audio_sample;
		t += 1.0f;
		sample = (int16_t)(INT16_MAX * sinf(input));
		s_tx_buffer[2 * i] = sample;
		s_tx_buffer[2 * i + 1] = sample;
	}

	ES8311_RegisterCb(onIdle, ES8311_ON_IDLE);
	ES8311_Transmit(s_tx_buffer, SSI_CHUNK_SIZE * 2);
#elif TEST_AUDIO_EN_MUSIC
	uint32_t i, remain;
	int16_t sample;

	LOG_D(__FUNCTION__, "PCM start: %u samples, %.1fs", PCM_DATA_NUM_SAMPLES, (float)PCM_DATA_NUM_SAMPLES / PCM_DATA_SAMPLE_RATE);

	/* 填第一个 buffer：mono → 展开为立体声 Left=Right */
	remain = PCM_DATA_NUM_SAMPLES - s_play_pos;
	for (i = 0; i < SSI_CHUNK_SIZE / 2; i++) {
		if (i < remain) {
			sample = g_pcm_data[s_play_pos + i];
		} else {
			sample = 0;
		}
		s_tx_buffer[2 * i]     = sample;
		s_tx_buffer[2 * i + 1] = sample;
	}
	s_play_pos += (remain < SSI_CHUNK_SIZE / 2) ? remain : (SSI_CHUNK_SIZE / 2);

	ES8311_SetVolume(25);
	ES8311_RegisterCb(onIdle, ES8311_ON_IDLE);
	ES8311_Transmit(s_tx_buffer, SSI_CHUNK_SIZE * 2);
#elif TEST_AUDIO_EN_MIC
	int ch;
	int volumn;

	s_has_record = 0;
	printf("ES8311 Mic Record / Playback Test\r\n");

	ES8311_RegisterCb(onIdle, ES8311_ON_IDLE);

	while (1) {
		printf("Input 1 to record\r\n");
		printf("Input 2 to play\r\n");
		printf("Input 3 to set volumn\r\n");
		printf("Input 4 to dump ADC registers\r\n");
		printf("Input 5 to exit\r\n");

		ch = getchar();
		while (getchar() != '\n') {}

		if (ch == '1') {
			LOG_D(__FUNCTION__, "Record start, target %ds", MIC_RECORD_SECS);

			s_recording = 1;
			ES8311_SetVolume(0);
			ES8311_Receive(s_record_buf, sizeof(s_record_buf));
			while (s_recording) {
				R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
			}
			s_has_record = 1;
			ES8311_SetVolume(40);

			LOG_D(__FUNCTION__, "Record done");
		}
		else if (ch == '2') {
			if (!s_has_record) {
				printf("No recording\r\n");
				continue;
			}

			LOG_D(__FUNCTION__, "Playing...");

			s_play_done = 0;
			ES8311_Transmit(s_record_buf, sizeof(s_record_buf));
			while (s_play_done == 0) {
				R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
			}

			LOG_D(__FUNCTION__, "Play done");
		}
		else if (ch == '3') {
			printf("Input new volumn: \r\n");
			scanf("%d", &volumn);
			printf("Set volumn: %d\r\n", volumn);
			ES8311_SetVolume((uint8_t)volumn);

			while (getchar() != '\n') {}
		}
		else if (ch == '4') {
			ES8311_DumpAdcRegs();
		}
		else if (ch == '5') {
			ES8311_RegisterCb(NULL, ES8311_ON_IDLE);

			break;
		}
		else {
			printf("Unsupport\r\n");
		}
	}
#endif

	return 0;
}

static void onIdle(void)
{
#if TEST_AUDIO_EN_SIN_WAVE
	ES8311_Transmit(s_tx_buffer, SSI_CHUNK_SIZE * 2);
#elif TEST_AUDIO_EN_MUSIC
	uint32_t i, remain;
	int16_t sample;

	remain = PCM_DATA_NUM_SAMPLES - s_play_pos;
	if (remain == 0) {
		LOG_D(__FUNCTION__, "Playback finished, looping");
		s_play_pos = 0;
		remain = PCM_DATA_NUM_SAMPLES;
	}

	for (i = 0; i < SSI_CHUNK_SIZE / 2; i++) {
		if (i < remain) {
			sample = g_pcm_data[s_play_pos + i];
		} else {
			sample = 0;
		}
		s_tx_buffer[2 * i]     = sample;
		s_tx_buffer[2 * i + 1] = sample;
	}
	s_play_pos += (remain < SSI_CHUNK_SIZE / 2) ? remain : (SSI_CHUNK_SIZE / 2);

	ES8311_Transmit(s_tx_buffer, SSI_CHUNK_SIZE * 2);
#elif TEST_AUDIO_EN_MIC
	if (s_recording) {
		s_recording = 0;
	}
	else {
		s_play_done = 1;
	}
#endif
}

#endif
