#ifndef __FUSB303B_H
#define __FUSB303B_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FUSB303B_ADDR	0x31

typedef enum {
	AUTO_PORTROLE_DRP,
	AUTO_PORTROLE_DRP_ONLY_SNK,
	AUTO_PORTROLE_DRP_ONLY_SRC,
	AUTO_PORTROLE_DRP_DONT_SAME_TIME,
	AUTO_PORTROLE_ONLY_SNK,
	AUTO_PORTROLE_ONLY_SRC
} FUSB303B_AutoPortroleEnum;

typedef enum {
	MANUAL_CONTROL_SRC,
	MANUAL_CONTROL_SNK,
	MANUAL_CONTROL_UNATTACHED_SNK,
	MANUAL_CONTROL_UNATTACHED_SRC,
	MANUAL_CONTROL_DISABLED,
	MANUAL_CONTROL_ERROR_RECOVERY
} FUSB303B_ManualControlEnum;

typedef enum {
	DETECT_TYPE_AUDIO,
	DETECT_TYPE_AUDIOVBUS,
	DETECT_TYPE_ACTIVECABLE,
	DETECT_TYPE_SOURCE,
	DETECT_TYPE_SINK,
	DETECT_TYPE_DEBUGSNK,
	DETECT_TYPE_DEBUGSRC,
	DETECT_TYPE_UNKNOW
} FUSB303B_DetectTypeEnum;

typedef enum {
	DETECT_CC_IN_CC1_A5,
	DETECT_CC_IN_CC2_B5,
	DETECT_CC_UNCONNECTION,
	DETECT_CC_FAULT
} FUSB303B_DetectCCOrientEnum;

typedef enum {
	CURRENT_LEVEL_UNCONNECTION,
	CURRENT_LEVEL_DEFAULT,
	CURRENT_LEVEL_1_DOT_5_A,
	CURRENT_LEVEL_3_DOT_0_A
} FUSB303B_CurrentLevel;

typedef struct {
	uint32_t attached : 1;
	uint32_t powerd_by_jusb : 1;
	uint32_t vbus_connect : 1;
	uint32_t : 29;
	FUSB303B_DetectTypeEnum type;
	FUSB303B_DetectCCOrientEnum cc_orient;
	FUSB303B_CurrentLevel current_level;
} FUSB303_Status;

void FUSB303B_Delay(uint32_t ms);
void FUSB303B_ENPinControl(uint8_t level);
void FUSB303B_Read(uint8_t addr, uint8_t *data, uint8_t length);
void FUSB303B_Write(uint8_t *data, uint8_t length);

uint32_t FUSB303B_Check(FUSB303_Status *status);
uint32_t FUSB303B_Init(void);
uint32_t FUSB303B_SetAutoPortrole(FUSB303B_AutoPortroleEnum role, bool en_audioacc);
uint32_t FUSB303B_SetCurrentLevel(FUSB303B_CurrentLevel level);
uint32_t FUSB303B_SetManualControl(FUSB303B_ManualControlEnum ctrl);

#ifdef __cplusplus
}
#endif

#endif
