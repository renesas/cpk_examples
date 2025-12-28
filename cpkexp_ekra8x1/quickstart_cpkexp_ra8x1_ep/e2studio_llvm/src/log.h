#ifndef __LOG_H
#define __LOG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_CFG_EN_SEGGER_RTT       1
#define LOG_CFG_EN_COLOR            0
#define LOG_CFG_EN_TIMESTAMP        0
#define LOG_CFG_FORMAT_SIZE         1024
#define LOG_CFG_LEVEL               LOG_LEVEL_DEBUG

#define LOG_LEVEL_DEBUG             0
#define LOG_LEVEL_INFO              1
#define LOG_LEVEL_WARN              2
#define LOG_LEVEL_ERROR             3
#define LOG_LEVEL_NONE              4

#if LOG_CFG_EN_SEGGER_RTT
void LOG_Puts(const char *str);
#else
extern void LOG_Puts(const char *str);
#endif

#if LOG_CFG_EN_TIMESTAMP
extern void LOG_GetTime(uint32_t *s, uint32_t *ms);
#endif

void LOG_Reset(void);
void LOG_PutsEndl(const char *str);
void LOG_Printf(const char *format, ...);
void LOG_PrintfEndl(const char *format, ...);

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
