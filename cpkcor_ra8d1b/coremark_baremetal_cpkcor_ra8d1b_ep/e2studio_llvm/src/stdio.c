#include <stdio.h>
#include <stdlib.h>

#include "SEGGER_RTT/SEGGER_RTT.h"

int segger_putchar(char ptr)
{
	if (ptr == '\n')
		SEGGER_RTT_PutChar(0, '\r');
	SEGGER_RTT_PutChar(0, ptr);

	return 0;
}

int segger_getchar(void)
{
	char ch;
	unsigned int size;
	
	do {
		size = SEGGER_RTT_Read(0, &ch, 1);
	} while (size == 0);

	return (int)ch;
}

#if defined(__ARMCC_VERSION)
int fputc(int ch, FILE *f)
{
	(void)f;
	return segger_putchar(ch);
}

int fgetc(FILE *f)
{
	(void)f;
	return segger_getchar();
}
#elif defined(__ICCARM__)
int __write(int fd, char *buf, int size)
{
	int i;

	(void)fd;
	for (i = 0; i < size; i++)
		segger_putchar(buf[i]);
}
#else
static int stdio_putchar(char ch, FILE *file)
{
	(void)file;
	return segger_putchar(ch);
}

static int stdio_getchar(FILE *file)
{
	(void)file;
	return segger_getchar();
}

static FILE __stdio = FDEV_SETUP_STREAM(stdio_putchar,
		stdio_getchar, NULL, _FDEV_SETUP_WRITE);
FILE *const stdin = &__stdio;
__strong_reference(stdin, stdout);
__strong_reference(stdin, stderr);
#endif
