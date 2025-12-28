#include <stdio.h>

#include "log.h"

#if LOG_CFG_LEVEL < LOG_LEVEL_NONE
static const char *sc_default = "\x1B[0m";
static const char *sc_clear = "\x1b[2J\x1b[H";
#if LOG_CFG_EN_COLOR
static const char *sc_grey = "\x1B[2m";
static const char *sc_yellow = "\x1B[0;33m";
static const char *sc_red = "\x1B[0;31m";
#endif
#endif

static char s_format_cache[LOG_CFG_FORMAT_SIZE];

static void logOutput(uint8_t level, const char *tag, const char *format, va_list args);

void LOG_PutsEndl(const char *str)
{
    if (str != NULL) {
        LOG_Puts(str);
    }
	LOG_Puts("\r\n");
}

void LOG_Printf(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vsnprintf(s_format_cache, LOG_CFG_FORMAT_SIZE, format, args);
	va_end(args);
	LOG_Puts(s_format_cache);
}

void LOG_PrintfEndl(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vsnprintf(s_format_cache, LOG_CFG_FORMAT_SIZE, format, args);
	va_end(args);
	LOG_PutsEndl(s_format_cache);
}

void LOG_Reset(void)
{
#if LOG_CFG_LEVEL < LOG_LEVEL_NONE
    LOG_PutsEndl(sc_default);
    LOG_PutsEndl(sc_clear);
#endif
}

#if LOG_CFG_LEVEL <= LOG_LEVEL_DEBUG
void LOG_D(const char *tag, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	logOutput(LOG_LEVEL_DEBUG, tag, format, args);
	va_end(args);
}
#endif

#if LOG_CFG_LEVEL <= LOG_LEVEL_INFO
void LOG_I(const char *tag, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	logOutput(LOG_LEVEL_INFO, tag, format, args);
	va_end(args);
}
#endif

#if LOG_CFG_LEVEL <= LOG_LEVEL_WARN
void LOG_W(const char *tag, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	logOutput(LOG_LEVEL_WARN, tag, format, args);
	va_end(args);
}
#endif

#if LOG_CFG_LEVEL <= LOG_LEVEL_ERROR
void LOG_E(const char *tag, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	logOutput(LOG_LEVEL_ERROR, tag, format, args);
	va_end(args);
}
#endif

static void logOutput(uint8_t level, const char *tag, const char *format, va_list args)
{
#if LOG_CFG_EN_TIMESTAMP
	uint32_t s, ms;

	LOG_GetTime(&s, &ms);
#endif

#if LOG_CFG_EN_COLOR
	if (level == LOG_LEVEL_DEBUG) {
		LOG_Puts(sc_grey);
	#if LOG_CFG_EN_TIMESTAMP
		LOG_Printf("[%d.%03d] ", s, ms);
	#endif
		LOG_Puts("DEBUG ");
	}
	else if (level == LOG_LEVEL_INFO) {
		LOG_Puts(sc_default);
	#if LOG_CFG_EN_TIMESTAMP
		LOG_Printf("[%d.%03d] ", s, ms);
	#endif
		LOG_Puts("INFO  ");
	}
	else if (level == LOG_LEVEL_WARN) {
		LOG_Puts(sc_yellow);
	#if LOG_CFG_EN_TIMESTAMP
		LOG_Printf("[%d.%03d] ", s, ms);
	#endif
		LOG_Puts("WARN  ");
	}
	else {
		LOG_Puts(sc_red);
	#if LOG_CFG_EN_TIMESTAMP
		LOG_Printf("[%d.%03d] ", s, ms);
	#endif
		LOG_Puts("ERROR ");
	}
#else
	if (level == LOG_LEVEL_DEBUG) {
	#if LOG_CFG_EN_TIMESTAMP
		LOG_Printf("[%d.%03d] ", s, ms);
	#endif
		LOG_Puts("DEBUG ");
	}
	else if (level == LOG_LEVEL_INFO) {
	#if LOG_CFG_EN_TIMESTAMP
		LOG_Printf("[%d.%03d] ", s, ms);
	#endif
		LOG_Puts("INFO  ");
	}
	else if (level == LOG_LEVEL_WARN) {
	#if LOG_CFG_EN_TIMESTAMP
		LOG_Printf("[%d.%03d] ", s, ms);
	#endif
		LOG_Puts("WARN  ");
	}
	else {
	#if LOG_CFG_EN_TIMESTAMP
		LOG_Printf("[%d.%03d] ", s, ms);
	#endif
		LOG_Puts("ERROR ");
	}
#endif

	if (tag != NULL) {
		LOG_Puts(tag);
		LOG_Puts(" ");
	}
	LOG_Puts("==> ");

	vsnprintf(s_format_cache, LOG_CFG_FORMAT_SIZE, format, args);
	LOG_PutsEndl(s_format_cache);
}

#if LOG_CFG_EN_SEGGER_RTT
#include "SEGGER_RTT/SEGGER_RTT.h"

void LOG_Puts(const char *str)
{
	int i;

	for (i = 0; str[i]; i++) {
		SEGGER_RTT_PutChar(0, str[i]);
	}
}
#else
void LOG_Puts(const char *str) __attribute__((weak));
void LOG_Puts(const char *str)
{
	(void)str;
}
#endif

#if LOG_CFG_EN_TIMESTAMP
void LOG_GetTime(uint32_t *s, uint32_t *ms) __attribute__((weak));
void LOG_GetTime(uint32_t *s, uint32_t *ms)
{
	*s = 0;
	*ms = 0;
}
#endif

