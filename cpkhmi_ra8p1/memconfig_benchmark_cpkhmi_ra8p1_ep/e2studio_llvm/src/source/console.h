#ifndef __CONSOLE_H
#define __CONSOLE_H

#include "hal_data.h"

/* Input/Output Stream Implementation
 * Only one option can be selected.
 * If CONSOLE_CFG_USE_CUSTOM is selected, The read and write functions need to be implemented.
 * USB/RPMSG/LCD have not been realized */
#ifndef CONSOLE_CFG_USE_RTT
#define CONSOLE_CFG_USE_RTT			0
#endif

#ifndef CONSOLE_CFG_USE_UART
#define CONSOLE_CFG_USE_UART		0
#endif

#ifndef CONSOLE_CFG_USE_USB
#define CONSOLE_CFG_USE_USB			0
#endif

#ifndef CONSOLE_CFG_USE_RPMSG
#define CONSOLE_CFG_USE_RPMSG		0
#endif

#ifndef CONSOLE_CFG_USE_LCD
#define CONSOLE_CFG_USE_LCD			0
#endif

#ifndef CONSOLE_CFG_USE_CUSTOM
#define CONSOLE_CFG_USE_CUSTOM		0
#endif

/* Receive cache size, only use in UART/USB/RPMSG */
#define CONSOLE_CFG_RX_BUF_SIZE		128

#if CONSOLE_CFG_USE_RTT
/* SEGGER RTT terminal index */
#define CONSOLE_CFG_RTT_INDEX		0
#endif

#if CONSOLE_CFG_USE_UART
/* UART instance and callback, it should be the same as FSP Configuration */
#define CONSOLE_CFG_UART_INSTANCE	g_uart9
#define CONSOLE_CFG_UART_CALLBACK	UART9_Callback
#endif

#if CONSOLE_CFG_USE_USB
/* USB instance and callback, it should be the same as FSP Configuration */
#define CONSOLE_CFG_USB_INSTANCE		g_usb_basic
#define CONSOLE_CFG_USB_CALLBACK		USB_Callback
#endif

#if CONSOLE_CFG_USE_RTT

#include "SEGGER_RTT/SEGGER_RTT.h"

#define CONSOLE_HasData				SEGGER_RTT_HasKey
#define CONSOLE_Init				SEGGER_RTT_Init
#define CONSOLE_Read(buffer, size)	SEGGER_RTT_Read(CONSOLE_CFG_RTT_INDEX, buffer, size)

#elif CONSOLE_CFG_USE_CUSTOM

extern int CONSOLE_HasData(void);
extern void CONSOLE_Init(void);
extern void CONSOLE_PutChar(char ch);
extern unsigned CONSOLE_Read(void* buffer, unsigned size);

#else

int CONSOLE_HasData(void);
void CONSOLE_Init(void);
unsigned CONSOLE_Read(void* buffer, unsigned size);

#endif /* #if CONSOLE_CFG_USE_RTT */

#endif
