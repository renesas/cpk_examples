#include "test.h"

#if TEST_EN_PDM

#include <stdio.h>
#include "es8311.h"
#include "hal_data.h"
#include "pdm.h"
#include "utils/log.h"

#define PDM_RECORD_SECS                 5U
#define PDM_SAMPLE_RATE                 44100U
#define PDM_CALLBACK_ALIGNMENT          8U
#define PDM_RECORD_FRAMES_RAW           (PDM_SAMPLE_RATE * PDM_RECORD_SECS)
#define PDM_RECORD_FRAMES               (((PDM_RECORD_FRAMES_RAW + PDM_CALLBACK_ALIGNMENT - 1U) / PDM_CALLBACK_ALIGNMENT) * PDM_CALLBACK_ALIGNMENT)
#define PDM_CAPTURE_BUFFER_FRAMES       (PDM_RECORD_FRAMES * 2U)

#define PDM_PLAYBACK_LEFT               0U
#define PDM_PLAYBACK_RIGHT              1U
#define PDM_PLAYBACK_STEREO             2U

/* Select PDM_PLAYBACK_LEFT, PDM_PLAYBACK_RIGHT, or PDM_PLAYBACK_STEREO. */
#define PDM_PLAYBACK_MODE               PDM_PLAYBACK_STEREO

#if ((PDM_PLAYBACK_MODE != PDM_PLAYBACK_LEFT) && \
     (PDM_PLAYBACK_MODE != PDM_PLAYBACK_RIGHT) && \
     (PDM_PLAYBACK_MODE != PDM_PLAYBACK_STEREO))
#error "Invalid PDM_PLAYBACK_MODE"
#endif

#if (PDM_PLAYBACK_MODE == PDM_PLAYBACK_LEFT)
#define PDM_PLAYBACK_MODE_NAME          "left"
#elif (PDM_PLAYBACK_MODE == PDM_PLAYBACK_RIGHT)
#define PDM_PLAYBACK_MODE_NAME          "right"
#else
#define PDM_PLAYBACK_MODE_NAME          "stereo"
#endif

/*
 * PDM always stores one right-justified sample in each uint32_t.  The second
 * half is a guard area: after the first-half callback, PDM can continue there
 * briefly while the foreground code stops reception without overwriting the
 * completed recording.
 */
static uint32_t s_record_left[PDM_CAPTURE_BUFFER_FRAMES] __attribute__((section(".sdram_noinit")));
static uint32_t s_record_right[PDM_CAPTURE_BUFFER_FRAMES] __attribute__((section(".sdram_noinit")));
static volatile uint8_t s_recording;
static volatile uint8_t s_left_done;
static volatile uint8_t s_right_done;
static volatile uint8_t s_record_error;
static volatile uint8_t s_play_done;
static uint8_t s_has_record;
static uint8_t s_volume = 30U;

static void onPdmLeftData(void);
static void onPdmRightData(void);
static void onPdmError(void);
static void onAudioIdle(void);
static int16_t pdmToPcm16(uint32_t raw);
static void preparePlaybackBuffer(void);

uint32_t TestPDM(void)
{
	int ch;
	int flush;
	int volume;
	uint32_t err;

	PDM_RegisterCb(onPdmLeftData, PDM_ON_LEFT_DATA);
	PDM_RegisterCb(onPdmRightData, PDM_ON_RIGHT_DATA);
	PDM_RegisterCb(onPdmError, PDM_ON_ERROR);
	ES8311_RegisterCb(onAudioIdle, ES8311_ON_IDLE);
	ES8311_SetVolume(s_volume);

	printf("PDM Mic Record / ES8311 Playback Test\r\n");
	printf("Playback mode: %s\r\n", PDM_PLAYBACK_MODE_NAME);

	while (1) {
		printf("Input 1 to record about %u seconds\r\n", PDM_RECORD_SECS);
		printf("Input 2 to play\r\n");
		printf("Input 3 to set volume\r\n");
		printf("Input 4 to exit\r\n");

		ch = getchar();
		do {
			flush = getchar();
		} while ((flush != '\n') && (flush != EOF));

		if (ch == '1') {
			LOG_D(__FUNCTION__, "PDM record start");
			s_has_record = 0;
			s_record_error = 0;
			s_left_done = 0;
			s_right_done = 0;
			s_recording = 1;
			ES8311_SetVolume(0);

			err = PDM_Start(s_record_left, s_record_right, sizeof(s_record_left), PDM_RECORD_FRAMES);
			if (err != 0) {
				s_recording = 0;
				ES8311_SetVolume(s_volume);
				LOG_E(__FUNCTION__, "PDM_Start() failed: %u", err);
				continue;
			}

			while (s_recording) {
				R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
			}

			err = PDM_Stop();
			if (err != 0) {
				ES8311_SetVolume(s_volume);
				LOG_E(__FUNCTION__, "PDM_Stop() failed: %u", err);
				continue;
			}

			if (s_record_error) {
				ES8311_SetVolume(s_volume);
				LOG_E(__FUNCTION__, "PDM recording aborted by hardware error");
				continue;
			}

			preparePlaybackBuffer();
			s_has_record = 1;
			ES8311_SetVolume(s_volume);
			LOG_D(__FUNCTION__, "PDM record done: %u frames, playback mode: %s", PDM_RECORD_FRAMES, PDM_PLAYBACK_MODE_NAME);
		}
		else if (ch == '2') {
			if (!s_has_record) {
				printf("No recording\r\n");
				continue;
			}

			LOG_D(__FUNCTION__, "Playing...");
			s_play_done = 0;
			err = ES8311_Transmit(s_record_left, PDM_RECORD_FRAMES * sizeof(uint32_t));
			if (err != 0) {
				LOG_E(__FUNCTION__, "ES8311_Transmit() failed: %u", err);
				continue;
			}

			while (!s_play_done) {
				R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
			}
			LOG_D(__FUNCTION__, "Play done");
		}
		else if (ch == '3') {
			printf("Input new volume [0, 100]: \r\n");
			if (scanf("%d", &volume) == 1) {
				if (volume < 0) {
					volume = 0;
				}
				else if (volume > 100) {
					volume = 100;
				}
				s_volume = (uint8_t) volume;
				ES8311_SetVolume(s_volume);
			}
			do {
				flush = getchar();
			} while ((flush != '\n') && (flush != EOF));
			printf("Set Volumn: %u\r\n", s_volume);
		}
		else if (ch == '4') {
			break;
		}
		else {
			printf("Unsupported input\r\n");
		}
	}

	PDM_RegisterCb(NULL, PDM_ON_LEFT_DATA);
	PDM_RegisterCb(NULL, PDM_ON_RIGHT_DATA);
	PDM_RegisterCb(NULL, PDM_ON_ERROR);
	ES8311_RegisterCb(NULL, ES8311_ON_IDLE);

	return 0;
}

static void onPdmLeftData(void)
{
	s_left_done = 1;
	if (s_right_done) {
		s_recording = 0;
	}
}

static void onPdmRightData(void)
{
	s_right_done = 1;
	if (s_left_done) {
		s_recording = 0;
	}
}

static void onPdmError(void)
{
	s_record_error = 1;
	s_recording = 0;
}

static void onAudioIdle(void)
{
	s_play_done = 1;
}

static int16_t pdmToPcm16(uint32_t raw)
{
	int32_t sample;

	/* Sign-extend the right-justified 20-bit PDM output. */
	sample = (int32_t)(raw & 0x000FFFFFU);
	if ((sample & 0x00080000L) != 0) {
		sample |= (int32_t)0xFFF00000U;
	}

	return (int16_t)(sample >> 4);
}

static void preparePlaybackBuffer(void)
{
#if (PDM_PLAYBACK_MODE != PDM_PLAYBACK_RIGHT)
	uint16_t left;
#endif
#if (PDM_PLAYBACK_MODE != PDM_PLAYBACK_LEFT)
	uint16_t right;
#endif
	uint32_t i;

	for (i = 0; i < PDM_RECORD_FRAMES; i++) {
#if (PDM_PLAYBACK_MODE == PDM_PLAYBACK_LEFT)
		/* Duplicate the left microphone so either I2S slot carries it. */
		left = (uint16_t)pdmToPcm16(s_record_left[i]);
		s_record_left[i] = (uint32_t)left | ((uint32_t)left << 16);
#elif (PDM_PLAYBACK_MODE == PDM_PLAYBACK_RIGHT)
		/* Duplicate the right microphone so either I2S slot carries it. */
		right = (uint16_t)pdmToPcm16(s_record_right[i]);
		s_record_left[i] = (uint32_t)right | ((uint32_t)right << 16);
#else
		/* Little-endian SSI buffer: left slot first, then right slot. */
		left = (uint16_t)pdmToPcm16(s_record_left[i]);
		right = (uint16_t)pdmToPcm16(s_record_right[i]);
		s_record_left[i] = (uint32_t)left | ((uint32_t)right << 16);
#endif
	}
}

#endif
