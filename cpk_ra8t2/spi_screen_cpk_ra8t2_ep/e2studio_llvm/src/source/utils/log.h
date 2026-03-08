#ifndef __LOG_H
#define __LOG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LOG_CFG_HAS_STDIO
#define LOG_CFG_HAS_STDIO			1
#endif

#ifndef LOG_CFG_EN_SEGGER_RTT
#define LOG_CFG_EN_SEGGER_RTT       0
#endif

#ifndef LOG_CFG_EN_COLOR
#define LOG_CFG_EN_COLOR            0
#endif

#ifndef LOG_CFG_EN_TIMESTAMP
#define LOG_CFG_EN_TIMESTAMP        0
#endif

#ifndef LOG_CFG_FORMAT_SIZE
#define LOG_CFG_FORMAT_SIZE         256
#endif

#ifndef LOG_CFG_LEVEL
#define LOG_CFG_LEVEL               LOG_LEVEL_DEBUG
#endif

#define LOG_LEVEL_DEBUG             0
#define LOG_LEVEL_INFO              1
#define LOG_LEVEL_WARN              2
#define LOG_LEVEL_ERROR             3
#define LOG_LEVEL_NONE              4

#if LOG_CFG_EN_TIMESTAMP
extern void LOG_GetTime(uint32_t *s, uint32_t *ms);
#endif

#if LOG_CFG_LEVEL < LOG_LEVEL_NONE

#if LOG_CFG_HAS_STDIO || LOG_CFG_EN_SEGGER_RTT
	#if LOG_CFG_HAS_STDIO
	#include "stdio.h"
	#endif
	void LOG_Puts(const char *str);
#else
extern void LOG_Puts(const char *str);
#endif

#define LOG_PutsEndl(str) { if (str != NULL) { LOG_Puts(str); } LOG_Puts("\r\n"); }

void LOG_Printf(const char *format, ...);
void LOG_PrintfEndl(const char *format, ...);

void LOG_Reset(void);
void LOG_SetAutoEndl(uint8_t val);
#else
#define LOG_Reset()
#define LOG_PutsEndl(str)
#define LOG_Printf(format, ...)
#define LOG_PrintfEndl(format, ...)
#endif

#if LOG_CFG_LEVEL <= LOG_LEVEL_DEBUG
void LOG_D(const char *tag, const char *format, ...) __attribute__((format(printf, 2, 3)));
#else
#define LOG_D(tag, format, ...)
#endif

#if LOG_CFG_LEVEL <= LOG_LEVEL_INFO
void LOG_I(const char *tag, const char *format, ...) __attribute__((format(printf, 2, 3)));
#else
#define LOG_I(tag, format, ...)
#endif

#if LOG_CFG_LEVEL <= LOG_LEVEL_WARN
void LOG_W(const char *tag, const char *format, ...) __attribute__((format(printf, 2, 3)));
#else
#define LOG_W(tag, format, ...)
#endif

#if LOG_CFG_LEVEL <= LOG_LEVEL_ERROR
void LOG_E(const char *tag, const char *format, ...) __attribute__((format(printf, 2, 3)));
#else
#define LOG_E(tag, format, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif
