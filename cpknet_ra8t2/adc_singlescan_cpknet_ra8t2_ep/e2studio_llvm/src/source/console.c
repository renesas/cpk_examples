#include <stdio.h>

#include "console.h"
#include "hal_data.h"

#if defined(__ARMCOMPILER_VERSION)

int fputc(int ch, FILE *f)
{
	(void)f;
	
#if CONSOLE_CFG_USE_RTT
    SEGGER_RTT_PutChar(CONSOLE_CFG_RTT_INDEX, ch);
#elif CONSOLE_CFG_USE_UART
    sci_b_uart_instance_ctrl_t *ctrl = (sci_b_uart_instance_ctrl_t *)CONSOLE_CFG_UART_INSTANCE.p_ctrl;
    ctrl->p_reg->TDR_BY = (uint8_t)ch;
    while ((ctrl->p_reg->CSR & R_SCI_B0_CSR_TDRE_Msk) == 0) {}
#elif CONSOLE_CFG_USE_USB
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_RPMSG
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_LCD
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_CUSTOM
    CONSOLE_PutChar(ch);
#endif
		
	return 0;
}

int fgetc(FILE *f)
{
	(void)f;
	
	char c;

	while (CONSOLE_HasData() == 0) {
	#ifdef __OPTIMIZE__
		__nop();
	#endif
	}
	CONSOLE_Read(&c, 1);

	return c;
}

#elif defined(__clang_version__)

static int __fputc(char ch, FILE *f)
{
	(void)ch;
    (void)f;

#if CONSOLE_CFG_USE_RTT
    SEGGER_RTT_PutChar(CONSOLE_CFG_RTT_INDEX, ch);
#elif CONSOLE_CFG_USE_UART
    sci_b_uart_instance_ctrl_t *ctrl = (sci_b_uart_instance_ctrl_t *)CONSOLE_CFG_UART_INSTANCE.p_ctrl;
    ctrl->p_reg->TDR_BY = ch;
    while ((ctrl->p_reg->CSR & R_SCI_B0_CSR_TDRE_Msk) == 0) {}
#elif CONSOLE_CFG_USE_USB
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_RPMSG
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_LCD
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_CUSTOM
    CONSOLE_PutChar(ch);
#endif

    return 0;
}

static int __fgetc(FILE *f)
{
	char c;

	(void)f;

	while (CONSOLE_HasData() == 0) {
	/* If use uart and compiler(ATfE 21.1.1) optimize,
	 * the compiler will generate incorrect assembly code here.
	 * So insert an __nop here to modify the behavior of compiler.
	 * See Snipaste_2025-12-23_17-47-24.png */
	#ifdef __OPTIMIZE__
		__nop();
	#endif
	}
	CONSOLE_Read(&c, 1);

	return c;
}

static FILE __stdio = FDEV_SETUP_STREAM(__fputc, __fgetc, NULL, _FDEV_SETUP_RW);
FILE * const stdin = &__stdio;
FILE * const stdout = &__stdio;
FILE * const stderr = &__stdio;

#else

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

int _close(int fd);
int _fstat(int fd, struct stat *st);
int _isatty(int fd);
int _lseek(int fd, int ptr, int dir);
int _read(int fd, char *pBuffer, int size);
int _write(int fd, char *pBuffer, int size);

__attribute__((weak)) int _close(int fd)
{
	if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
		return 0;
	}

	errno = EBADF;

	return -1;
}

__attribute__((weak)) int _fstat(int fd, struct stat *st)
{
	if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
		st->st_mode = S_IFCHR;
	}
	errno = EBADF;

	return 0;
}

__attribute__((weak)) int _isatty(int fd)
{
	if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
		return 1;
	}

	errno = EBADF;

	return 0;
}

__attribute__((weak)) int _lseek(int fd, int ptr, int dir)
{
	(void)fd;
	(void)ptr;
	(void)dir;

	errno = EBADF;

	return -1;
}

int _read(int fd, char *pBuffer, int size)
{
	unsigned r;

	unsigned have_read = 0;
	unsigned target = (unsigned)size;
	char *ptr = pBuffer;

	(void)fd;

	while (have_read < target) {
		while (CONSOLE_HasData() == 0) {
		#ifdef __OPTIMIZE__
			__NOP();
		#endif
		}
		r = CONSOLE_Read(ptr, target);
		have_read += r;
	}

	return size;
}

int _write(int fd, char *pBuffer, int size)
{
	(void)fd;

#if CONSOLE_CFG_USE_RTT
    SEGGER_RTT_Write(CONSOLE_CFG_RTT_INDEX, pBuffer, (unsigned)size);
#elif CONSOLE_CFG_USE_UART
    sci_b_uart_instance_ctrl_t *ctrl = (sci_b_uart_instance_ctrl_t *)CONSOLE_CFG_UART_INSTANCE.p_ctrl;
    for (int i = 0; i < size; i++) {
    		ctrl->p_reg->TDR_BY = (uint8_t)pBuffer[i];
    		while ((ctrl->p_reg->CSR & R_SCI_B0_CSR_TDRE_Msk) == 0) {}
    }
#elif CONSOLE_CFG_USE_USB
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_RPMSG
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_LCD
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_CUSTOM
    CONSOLE_PutChar(ch);
#endif

	return size;
}

#endif

#if CONSOLE_CFG_USE_RTT

#elif CONSOLE_CFG_USE_CUSTOM

int CONSOLE_HasData(void) __attribute__((weak));
int CONSOLE_HasData(void)
{
	return 0;
}

void CONSOLE_Init(void) __attribute__((weak));
void CONSOLE_Init(void)
{}

void CONSOLE_PutChar(char ch) __attribute__((weak));
void CONSOLE_PutChar(char ch)
{
	(void)ch;
}

void CONSOLE_Read(void* buffer, unsigned size) __attribute__((weak));
void CONSOLE_Read(void* buffer, unsigned size)
{
	(void)buffer;
	(void)size;
}

#else

static uint8_t s_rx_buf[CONSOLE_CFG_RX_BUF_SIZE];
static uint32_t s_head_index;
static uint32_t s_tail_index;
/**
 * @brief   Checks if at least one character for reading is available at rx buf
 * @return  0: no data, 1: data avaliable
 */
int CONSOLE_HasData(void)
{
    return s_head_index != s_tail_index ? 1 : 0;
}

/**
 * @brief	Init io peripheral
 * @return	0: Success, other: failed
 */
void CONSOLE_Init(void)
{
	memset(s_rx_buf, 0, CONSOLE_CFG_RX_BUF_SIZE);
	s_head_index = 0;
	s_tail_index = 0;
#if CONSOLE_CFG_USE_UART
	R_SCI_B_UART_Open(CONSOLE_CFG_UART_INSTANCE.p_ctrl, CONSOLE_CFG_UART_INSTANCE.p_cfg);
#elif CONSOLE_CFG_USE_USB
	/* TODO Wait implementation */
#elif CONSOLE_CFG_USE_RPMSG
	/* TODO Wait implementation */
#elif CONSOLE_CFG_USE_LCD
	/* TODO Wait implementation */
#endif
}

/**
 * @brief   Read data from rx buf
 * @param   buffer Data will be saved here
 * @param   size   Receive size
 * @return  Actual size read
 */
unsigned CONSOLE_Read(void* buffer, unsigned size)
{
    unsigned read_num;
    uint32_t rx_length, remain;

    uint8_t *p8 = (uint8_t *)buffer;

    if (s_head_index < s_tail_index) {
        rx_length = s_tail_index - s_head_index;
        read_num = rx_length > size ? size : rx_length;
        memcpy(p8, &s_rx_buf[s_head_index], read_num);
        s_tail_index -= read_num;
    }
    else if (s_head_index == s_tail_index) {
        read_num = 0;
    }
    else {
        rx_length = CONSOLE_CFG_RX_BUF_SIZE - (s_head_index - s_tail_index);
        read_num = rx_length > size ? size : rx_length;

        memcpy(p8, &s_rx_buf[s_head_index], CONSOLE_CFG_RX_BUF_SIZE - s_head_index);
        if (read_num < (CONSOLE_CFG_RX_BUF_SIZE - s_head_index)) {
            s_head_index += read_num;
            if (s_head_index == CONSOLE_CFG_RX_BUF_SIZE) {
                s_head_index = 0;
            }
        }
        else {
            remain = read_num - (CONSOLE_CFG_RX_BUF_SIZE - s_head_index);
            memcpy(&p8[CONSOLE_CFG_RX_BUF_SIZE - s_head_index], s_rx_buf, remain);
            s_head_index += remain;
        }
    }

    return read_num;
}

#if CONSOLE_CFG_USE_UART
void CONSOLE_CFG_UART_CALLBACK(uart_callback_args_t *p_args)
{
    if (p_args->event == UART_EVENT_RX_CHAR) {
        s_rx_buf[s_tail_index] = (uint8_t)p_args->data;
        s_tail_index++;
        if (s_tail_index == CONSOLE_CFG_RX_BUF_SIZE) {
            s_tail_index = 0;
        }
        if (s_tail_index == s_head_index) {
            s_head_index++;
            if (s_head_index == CONSOLE_CFG_RX_BUF_SIZE) {
                s_head_index = 0;
            }
        }
    }
}
#elif CONSOLE_CFG_USE_USB
/* TODO Wait implementation */
#elif CONSOLE_CFG_USE_RPMSG
/* TODO Wait implementation */
#endif

#endif /* #if CONSOLE_CFG_USE_RTT == 0 */
