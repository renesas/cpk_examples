#include "softiic.h"

struct SoftiicFlagsBits {
    uint8_t re_config : 1;
    uint8_t reserved : 7;
};

union SoftiicFlags {
    struct SoftiicFlagsBits bits;
    uint8_t value;
};

static union SoftiicFlags s_flags;

static void softiicGenStart(SoftiicType *siic);
static void softiicGenStop(SoftiicType *siic);
static uint8_t softiicWaitAck(SoftiicType *siic);
static void softiicGenAck(SoftiicType *siic);
static void softiicGenNoAck(SoftiicType *siic);
static void softiicSendByte(SoftiicType *siic, uint8_t data);
static uint8_t softiicReadByte(SoftiicType *siic, uint8_t ack);

static void softiicGenStart(SoftiicType *siic)
{
    if (s_flags.bits.re_config) {
        SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
        SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    }

    SoftiicSDAHigh(siic->group_sda, siic->pin_sda);
    SoftiicSCLHigh(siic->group_scl, siic->pin_scl);
    SoftiicDelayUs(siic->time);
    SoftiicSDALow(siic->group_sda, siic->pin_sda);
    SoftiicDelayUs(siic->time);
    SoftiicSCLLow(siic->group_scl, siic->pin_scl);
}

static void softiicGenStop(SoftiicType *siic)
{
    if (s_flags.bits.re_config) {
        SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
        SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    }

    SoftiicSCLLow(siic->group_scl, siic->pin_scl);
    SoftiicSDALow(siic->group_sda, siic->pin_sda);
    SoftiicDelayUs(siic->time);
    SoftiicSCLHigh(siic->group_scl, siic->pin_scl);
    SoftiicSDAHigh(siic->group_sda, siic->pin_sda);
    SoftiicDelayUs(siic->time);
}

static uint8_t softiicWaitAck(SoftiicType *siic)
{
    uint32_t timeout = 0;

    if (s_flags.bits.re_config) {
        SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
        SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    }

    SoftiicSDAHigh(siic->group_sda, siic->pin_sda);
    SoftiicDelayUs(2);
    SoftiicSCLHigh(siic->group_scl, siic->pin_scl);
    SoftiicDelayUs(4);
    SoftiicSetPinAsInput(siic->group_sda, siic->pin_sda);
    while (SoftiicGetSDALevel(siic->group_sda, siic->pin_sda)) {
        SoftiicDelayUs(1);
        timeout++;
        if (timeout > 2000) {
            SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
            softiicGenStop(siic);
            return 1;
        }
    }
    SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    SoftiicSCLLow(siic->group_scl, siic->pin_scl);

    return 0;
}

static void softiicGenAck(SoftiicType *siic)
{
    if (s_flags.bits.re_config) {
        SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
        SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    }

    SoftiicSCLLow(siic->group_scl, siic->pin_scl);
    SoftiicSDALow(siic->group_sda, siic->pin_sda);
    SoftiicDelayUs(2);
    SoftiicSCLHigh(siic->group_scl, siic->pin_scl);
    SoftiicDelayUs(2);
    SoftiicSCLLow(siic->group_scl, siic->pin_scl);
}

static void softiicGenNoAck(SoftiicType *siic)
{
    if (s_flags.bits.re_config) {
        SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
        SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    }

    SoftiicSCLLow(siic->group_scl, siic->pin_scl);
    SoftiicSDAHigh(siic->group_sda, siic->pin_sda);
    SoftiicDelayUs(2);
    SoftiicSCLHigh(siic->group_scl, siic->pin_scl);
    SoftiicDelayUs(2);
    SoftiicSCLLow(siic->group_scl, siic->pin_scl);
}

static void softiicSendByte(SoftiicType *siic, uint8_t data)
{
    uint8_t i;

    if (s_flags.bits.re_config) {
        SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
        SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    }

    SoftiicSCLLow(siic->group_scl, siic->pin_scl);
    for (i = 0; i < 8; i++) {
        if (((data & 0x80) >> 7) != 0) {
            SoftiicSDAHigh(siic->group_sda, siic->pin_sda);
        }
        else {
            SoftiicSDALow(siic->group_sda, siic->pin_sda);
        }
        data <<= 1;
        SoftiicDelayUs(2);
        SoftiicSCLHigh(siic->group_scl, siic->pin_scl);
        SoftiicDelayUs(2);
        SoftiicSCLLow(siic->group_scl, siic->pin_scl);
        SoftiicDelayUs(2);
    }
}

static uint8_t softiicReadByte(SoftiicType *siic, uint8_t ack)
{
    uint8_t i;
    uint8_t result = 0;

    if (s_flags.bits.re_config) {
        SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
        SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    }

    SoftiicSetPinAsInput(siic->group_sda, siic->pin_sda);
    for (i = 0; i < 8; i++) {
        SoftiicSCLLow(siic->group_scl, siic->pin_scl);
        SoftiicDelayUs(2);
        SoftiicSCLHigh(siic->group_scl, siic->pin_scl);
        result <<= 1;
        if (SoftiicGetSDALevel(siic->group_sda, siic->pin_sda)) {
            result++;
        }
        SoftiicDelayUs(1);
    }

    /* Bring SCL low BEFORE switching SDA to output. Otherwise, if the LSB just
     * read was 1 (SDA high), switching SDA to output-low while SCL is still
     * high creates a false START condition that corrupts the EEPROM state. */
    SoftiicSCLLow(siic->group_scl, siic->pin_scl);
    SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    if (ack) {
        softiicGenAck(siic);
    }
    else {
        softiicGenNoAck(siic);
    }

    return result;
}

uint32_t SoftiicWriteArray(SoftiicType *siic, uint8_t slave, uint8_t *array, uint16_t length)
{
    uint16_t i;

    SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
    SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    s_flags.bits.re_config = 0;

    softiicGenStart(siic);
    softiicSendByte(siic, (uint8_t)(slave << 1));
    if (softiicWaitAck(siic)) {
        return 1;
    }

    for (i = 0; i < length; i++) {
        softiicSendByte(siic, array[i]);
        if (softiicWaitAck(siic)) {
            return 1;
        }
    }
    softiicGenStop(siic);

    return 0;
}

uint32_t SoftiicWriteMem(SoftiicType *siic, uint8_t slave, uint16_t mem_addr, uint8_t mem_size, uint8_t *data, uint16_t length)
{
    uint16_t i;

    SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
    SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    s_flags.bits.re_config = 0;

    softiicGenStart(siic);
    softiicSendByte(siic, (uint8_t)(slave << 1));
    if (softiicWaitAck(siic)) {
        return 1;
    }
    if (mem_size == 8) {
        softiicSendByte(siic, mem_addr & 0x00FF);
        if (softiicWaitAck(siic)) {
            return 2;
        }
    }
    else if (mem_size == 16) {
        softiicSendByte(siic, (mem_addr >> 8) & 0x00FF);
        if (softiicWaitAck(siic)) {
            return 2;
        }
        softiicSendByte(siic, mem_addr & 0x00FF);
        if (softiicWaitAck(siic)) {
            return 2;
        }
    }
    else {
        softiicGenStop(siic);
        return 4;
    }
    for (i = 0; i < length; i++) {
        softiicSendByte(siic, data[i]);
        if (softiicWaitAck(siic)) {
            return 3;
        }
    }
    softiicGenStop(siic);

    return 0;
}

uint32_t SoftiicWriteReg(SoftiicType *siic, uint8_t slave, uint16_t reg_addr, uint8_t reg_width, uint16_t val, uint8_t val_width)
{
	SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
    SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    s_flags.bits.re_config = 0;

    softiicGenStart(siic);
    softiicSendByte(siic, (uint8_t)(slave << 1));
    if (softiicWaitAck(siic)) {
        return 1;
    }

    if (reg_width == 8) {
    	softiicSendByte(siic, reg_addr & 0xFF);
    	if (softiicWaitAck(siic)) {
            return 2;
        }
    }
    else {
    	softiicSendByte(siic, (reg_addr >> 8) & 0xFF);
    	if (softiicWaitAck(siic)) {
            return 2;
        }
    	softiicSendByte(siic, reg_addr & 0xFF);
    	if (softiicWaitAck(siic)) {
            return 2;
        }
    }

    if (val_width == 8) {
    	softiicSendByte(siic, val & 0xFF);
    	if (softiicWaitAck(siic)) {
            return 3;
        }
    }
    else {
    	softiicSendByte(siic, (val >> 8) & 0xFF);
    	if (softiicWaitAck(siic)) {
            return 3;
        }
    	softiicSendByte(siic, val & 0xFF);
    	if (softiicWaitAck(siic)) {
            return 3;
        }
    }

    softiicGenStop(siic);

	return 0;
}

uint32_t SoftiicReadArray(SoftiicType *siic, uint8_t slave, uint8_t *array, uint16_t length)
{
    uint16_t i;

    SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
    SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    s_flags.bits.re_config = 0;

    softiicGenStart(siic);
    softiicSendByte(siic, (uint8_t)((slave << 1) | 0x01));
    if (softiicWaitAck(siic)) {
        return 1;
    }
    for (i = 0; i < length; i++) {
        if (i == (length - 1)) {
            array[i] = softiicReadByte(siic, 0);
        }
        else {
            array[i] = softiicReadByte(siic, 1);
        }
    }
    softiicGenStop(siic);

    return 0;
}

uint32_t SoftiicWaitDevice(SoftiicType *siic, uint8_t slave, uint32_t timeout_ms)
{
	SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
	SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
	s_flags.bits.re_config = 0;

	while (timeout_ms--) {
		softiicGenStart(siic);
		softiicSendByte(siic, (uint8_t)(slave << 1));
		if (softiicWaitAck(siic) == 0) {
			softiicGenStop(siic);
			return 0;
		}
		SoftiicDelayUs(1000);
	}
	return 1;
}

uint32_t SoftiicReadMem(SoftiicType *siic, uint8_t slave, uint16_t mem_addr, uint8_t mem_size, uint8_t *data, uint16_t length)
{
    uint16_t i;

    SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
    SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    s_flags.bits.re_config = 0;

    softiicGenStart(siic);
    softiicSendByte(siic, (uint8_t)(slave << 1));
    if (softiicWaitAck(siic)) {
        return 1;
    }
    if (mem_size == 8) {
        softiicSendByte(siic, mem_addr & 0x00FF);
        if (softiicWaitAck(siic)) {
            return 2;
        }
    }
    else if (mem_size == 16) {
        softiicSendByte(siic, (mem_addr >> 8) & 0x00FF);
        if (softiicWaitAck(siic)) {
            return 2;
        }
        softiicSendByte(siic, mem_addr & 0x00FF);
        if (softiicWaitAck(siic)) {
            return 2;
        }
    }
    else {
        softiicGenStop(siic);
        return 4;
    }

    softiicGenStart(siic);
    softiicSendByte(siic, (uint8_t)((slave << 1) | 0x01));
    if (softiicWaitAck(siic)) {
        return 3;
    }
    for (i = 0; i < length; i++) {
        if (i == (length - 1)) {
            data[i] = softiicReadByte(siic, 0);
        }
        else {
            data[i] = softiicReadByte(siic, 1);
        }
    }
    softiicGenStop(siic);

    return 0;
}

uint32_t SoftiicReadReg(SoftiicType *siic, uint8_t slave, uint16_t reg_addr, uint8_t reg_width, uint8_t *val, uint8_t val_width)
{
	SoftiicSetPinAsOD(siic->group_scl, siic->pin_scl);
    SoftiicSetPinAsOD(siic->group_sda, siic->pin_sda);
    s_flags.bits.re_config = 0;

    softiicGenStart(siic);
    softiicSendByte(siic, (uint8_t)(slave << 1));
    if (softiicWaitAck(siic)) {
        return 1;
    }

    if (reg_width == 8) {
    	softiicSendByte(siic, reg_addr & 0xFF);
    	if (softiicWaitAck(siic)) {
            return 2;
        }
    }
    else {
    	softiicSendByte(siic, (reg_addr >> 8) & 0xFF);
    	if (softiicWaitAck(siic)) {
            return 2;
        }
    	softiicSendByte(siic, reg_addr & 0xFF);
    	if (softiicWaitAck(siic)) {
            return 2;
        }
    }

    softiicGenStart(siic);
    softiicSendByte(siic, (uint8_t)((slave << 1) | 0x01));
    if (softiicWaitAck(siic)) {
        return 3;
    }
    if (val_width == 8) {
    	val[0] = softiicReadByte(siic, 0);
    }
    else {
    	val[0] = softiicReadByte(siic, 1);
    	val[1] = softiicReadByte(siic, 0);
    }
    softiicGenStop(siic);

	return 0;
}
