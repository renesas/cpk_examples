/*
* This source file is part of the EtherCAT Slave Stack Code licensed by Beckhoff Automation GmbH & Co KG, 33415 Verl, Germany.
* The corresponding license agreement applies. This hint shall not be removed.
*/

/**
\addtogroup SampleAppl Sample Application
@{
*/

/**
\file Sampleappl.c
\author EthercatSSC@beckhoff.com
\brief Implementation

\version 5.12

<br>Changes to version V5.11:<br>
V5.12 ECAT1: update SM Parameter measurement (based on the system time), enhancement for input only devices and no mailbox support, use only 16Bit pointer in process data length caluclation<br>
V5.12 ECAT2: big endian changes<br>
V5.12 EOE1: move icmp sample to the sampleappl,add EoE application interface functions<br>
V5.12 EOE2: prevent static ethernet buffer to be freed<br>
V5.12 EOE3: fix memory leaks in sample ICMP application<br>
V5.12 FOE1: update new interface,move the FoE sample to sampleappl,add FoE application callback functions<br>
<br>Changes to version V5.10:<br>
V5.11 ECAT11: create application interface function pointer, add eeprom emulation interface functions<br>
V5.11 ECAT4: enhance SM/Sync monitoring for input/output only slaves<br>
<br>Changes to version V5.01:<br>
V5.10 ECAT10: Add missing include 'objdef.h'<br>
              Add process data size calculation to sampleappl<br>
V5.10 ECAT6: Add "USE_DEFAULT_MAIN" to enable or disable the main function<br>
V5.10 FC1100: Stop stack if hardware init failed<br>
<br>Changes to version V5.0:<br>
V5.01 APPL2: Update Sample Application Output mapping<br>
V5.0: file created
*/

/*-----------------------------------------------------------------------------------------
------
------    Includes
------
-----------------------------------------------------------------------------------------*/
#include "hal_data.h"
#include "ecat_def.h"
#include "applInterface.h"
#include "cia402appl.h"
#include "samplecia402.h"
#include "sampleios.h"

#if defined(BOARD_RA8T2_EK)
#include "i2c_expander.h"
#endif

#if (CiA402_SAMPLE_APPLICATION == 1)
/*--------------------------------------------------------------------------------------
------
------    local types and defines
------
--------------------------------------------------------------------------------------*/
typedef struct
{
	int32_t		TargetPosition;
	int32_t		PositionActualValue;
	int32_t		TargetVelocity;
	int32_t		VelocityActualValue;
}TAxis;

/*-----------------------------------------------------------------------------------------
------
------    local variables and constants
------
-----------------------------------------------------------------------------------------*/
extern TCiA402Axis       LocalAxes[MAX_AXES];

static TAxis LocalTAxis[MAX_AXES];

/*-----------------------------------------------------------------------------------------
------
------    application specific functions
------
-----------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------
------
------    generic functions
------
-----------------------------------------------------------------------------------------*/

/* Function Name: DummyMotor */
/******************************************************************************************************************//**
 * @brief Increment VelocityActualValue
 *********************************************************************************************************************/
void DummyMotor(void)
{

	TCiA402Axis *pCiA402Axis;
	uint8_t i;

	for(i = 0; i < MAX_AXES;i++)
	{
		pCiA402Axis = &LocalAxes[i];
		switch( pCiA402Axis->Objects.objModesOfOperationDisplay )
		{
		case CYCLIC_SYNC_POSITION_MODE:
			if(pCiA402Axis->i16State == STATE_OPERATION_ENABLED)
			{
				if(LocalTAxis[i].PositionActualValue < LocalTAxis[i].TargetPosition)
				{
					LocalTAxis[i].PositionActualValue++;
				}
			}
			else
			{
				LocalTAxis[i].PositionActualValue = 0;
			}
			break;
		case CYCLIC_SYNC_VELOCITY_MODE:
			if(pCiA402Axis->i16State == STATE_OPERATION_ENABLED)
			{
				if(LocalTAxis[i].VelocityActualValue < LocalTAxis[i].TargetVelocity)
				{
					LocalTAxis[i].VelocityActualValue++;
				}
			}
			else
			{
				LocalTAxis[i].VelocityActualValue = 0;
			}
			break;
		default:
			break;
		}
	}
}

/* Function Name: CiA402_DummyMotionControl */
/******************************************************************************************************************//**
 * @brief this functions provides an simple feedback functionality
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @param[in] i CiA402 axis number.
 *********************************************************************************************************************/
void CiA402_DummyMotionControl(TCiA402Axis *pCiA402Axis, UINT16 i)
{
	/************* DUMMY MOTION CONTROL ****************/

    /* update actual value's */
    pCiA402Axis->Objects.objPositionActualValue = LocalTAxis[i].PositionActualValue;
    pCiA402Axis->Objects.objVelocityActualValue = LocalTAxis[i].VelocityActualValue;

	/* if axis function enabled */
	if(	((pCiA402Axis->bAxisFunctionEnabled) && (pCiA402Axis->bLowLevelPowerApplied))
		&& ((pCiA402Axis->bHighLevelPowerApplied) && !(pCiA402Axis->bBrakeApplied)) )
	{
		/* execute mode request										*/
		/*----------------------------------------------------------*/
		switch( pCiA402Axis->Objects.objModesOfOperationDisplay )
		{
			/* position mode */
			case CYCLIC_SYNC_POSITION_MODE:
				/* if not exceed internal limits */
				if((pCiA402Axis->Objects.objSoftwarePositionLimit.i32MaxLimit> pCiA402Axis->Objects.objPositionActualValue
					|| pCiA402Axis->Objects.objPositionActualValue > pCiA402Axis->Objects.objTargetPosition) &&
					(pCiA402Axis->Objects.objSoftwarePositionLimit.i32MinLimit < pCiA402Axis->Objects.objPositionActualValue
					|| pCiA402Axis->Objects.objPositionActualValue < pCiA402Axis->Objects.objTargetPosition))
				{
					/* clear internal limit flag */
					pCiA402Axis->Objects.objStatusWord &= (UINT16)~STATUSWORD_INTERNAL_LIMIT;
					/* execute position control */
					LocalTAxis[i].TargetPosition = pCiA402Axis->Objects.objTargetPosition;
				}else{
					/* set internal limit flag */
					pCiA402Axis->Objects.objStatusWord |= STATUSWORD_INTERNAL_LIMIT;
				}
			break;

			/* velocity mode */
			case CYCLIC_SYNC_VELOCITY_MODE:
			    /* execute velocity control */
				LocalTAxis[i].TargetVelocity = pCiA402Axis->Objects.objTargetVelocity;
			break;
			/* torque mode */
			case CYCLIC_SYNC_TORQUE_MODE:
			break;
			/* other */
			default:
				/* do nothing (not supported) */
			break;
		}
	}

	/* Accept new mode of operation */
	pCiA402Axis->Objects.objModesOfOperationDisplay = pCiA402Axis->Objects.objModesOfOperation;

}

/* Function Name: CiA402_StateTransition1 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 1 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition1(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition1 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition2 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 2 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition2(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition2 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition3 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 3 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition3(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition3 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition4 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 4 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition4(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition4 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition5 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 5 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition5(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition5 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition6 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 6 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition6(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition6 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition7 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 7 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition7(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition7 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition8 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 8 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition8(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition8 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition9 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 9 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition9(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition9 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition10 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 10 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition10(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition10 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition11 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 11 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition11(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition11 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition12 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 12 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition12(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition12 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_LocalError */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 13 has occurred. Describe the operation in the case of the state transition.
 * @param[in] ErrorCode
 *********************************************************************************************************************/
void CiA402_LocalError(UINT16 ErrorCode)
{
    UINT16 counter = 0;
    for(counter = 0; counter < MAX_AXES; counter++)
    {
        if(LocalAxes[counter].bAxisIsActive)
        {
            LocalAxes[counter].i16State = STATE_FAULT_REACTION_ACTIVE;
            LocalAxes[counter].Objects.objErrorCode = ErrorCode;
        }
    }
#if(_DUMMY_ == 1)
    printf("Axis1 :StateTransition13 Pass\n");
    printf("Axis2 :StateTransition13 Pass\n");
#endif
}

/* Function Name: CiA402_StateTransition14 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 14 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition14(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition14 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition15 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 15 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition15(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition15 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: CiA402_StateTransition16 */
/******************************************************************************************************************//**
 * @brief This function is used when state transition 16 has occurred. Describe the operation in the case of the state transition.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @return Result of the state transition.
 * @retval 0 Normal end. Transition completed successfully.
 * @retval 1 Error. Transition failed due to an error.
 *********************************************************************************************************************/
UINT16 CiA402_StateTransition16(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition16 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	FSP_PARAMETER_NOT_USED(pCiA402Axis);
	return 0;
}

/* Function Name: APPL_MOTOR_MotionControl_Main */
/******************************************************************************************************************//**
 * @brief Implement the motion control code when the state of CiA402 FSA is "Operation enabled".
 *        Describe the process for each mode of operation.
 * @param[in] pCiA402Axis Pointer to the CiA402 axis structure.
 * @param[in] i The CiA402 axis number.
 * @return Result.
 * @retval 0 Normal end.
 * @retval 1 Error.
 *********************************************************************************************************************/
UINT16 APPL_MOTOR_MotionControl_Main(TCiA402Axis *pCiA402Axis, UINT16 i)
{
	CiA402_DummyMotionControl(pCiA402Axis, i);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \return    The Explicit Device ID of the EtherCAT slave

 \brief     Calculate the Explicit Device ID
*////////////////////////////////////////////////////////////////////////////////////////
UINT16 APPL_GetDeviceID()
{
#if defined(BOARD_RA8T2_EK)
	return ((APPL_GetDipSw() >> 4) & 0x000f);			// High 4bit
#elif defined(BOARD_RA8T2_MCK)
	return (APPL_GetDipSw());
#elif defined(BOARD_RA8T2_CPKNET)
	return (APPL_GetDipSw());
#else
    return 0x5;
#endif //  defined(BOARD_RA8T2_MCK)
}

#if defined(BOARD_RA8T2_MCK) | defined(BOARD_RA8T2_EK) | defined(BOARD_RA8T2_CPKNET)
/////////////////////////////////////////////////////////////////////////////////////////
/**
 \param    UINT16 LED output value. The value one means ON.

 \brief    SET LED
*////////////////////////////////////////////////////////////////////////////////////////
void APPL_SetLed(UINT16 value)
{
	/* LED type structure */
	sample_leds_t leds = g_sample_leds;

    /* Holds level to set for pins */
    bsp_io_level_t pin_level[4];

	/* This code uses BSP IO functions to show how it is used.*/
	R_BSP_PinAccessEnable();
#if defined(BOARD_RA8T2_MCK)
    pin_level[SAMPLE_LED_RLED0] = ((value & 1) ?  BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
    pin_level[SAMPLE_LED_RLED1] = ((value & 2) ?  BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
    pin_level[SAMPLE_LED_RLED2] = ((value & 4) ?  BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
    pin_level[SAMPLE_LED_RLED3] = ((value & 8) ?  BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
#endif

#if defined(BOARD_RA8T2_CPKNET)
	pin_level[SAMPLE_LED_RLED0] = ((value & 1) ?  BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH);
	pin_level[SAMPLE_LED_RLED1] = ((value & 2) ?  BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH);
	pin_level[SAMPLE_LED_RLED2] = ((value & 4) ?  BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH);
	pin_level[SAMPLE_LED_RLED3] = ((value & 8) ?  BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH);
#endif

#if defined(BOARD_RA8T2_MCK) | defined(BOARD_RA8T2_EK) | defined(BOARD_RA8T2_CPKNET)
	R_IOPORT_PinWrite(g_ioport.p_ctrl, (bsp_io_port_pin_t)leds.p_leds[SAMPLE_LED_RLED0], pin_level[SAMPLE_LED_RLED0]);
	R_IOPORT_PinWrite(g_ioport.p_ctrl, (bsp_io_port_pin_t)leds.p_leds[SAMPLE_LED_RLED1], pin_level[SAMPLE_LED_RLED1]);
	R_IOPORT_PinWrite(g_ioport.p_ctrl, (bsp_io_port_pin_t)leds.p_leds[SAMPLE_LED_RLED2], pin_level[SAMPLE_LED_RLED2]);
#if defined(BOARD_RA8T2_MCK)
	R_IOPORT_PinWrite(g_ioport.p_ctrl, (bsp_io_port_pin_t)leds.p_leds[SAMPLE_LED_RLED3], pin_level[SAMPLE_LED_RLED3]);
#endif // defined(BOARD_RA8T2_MCK)
#endif // defined(BOARD_RA8T2_MCK) | defined(BOARD_RA8T2_EK)
	/* Protect PFS registers */
	R_BSP_PinAccessDisable();
}
/////////////////////////////////////////////////////////////////////////////////////////
/**
 \retuen   UINT16 DIP SW value. Low input level means ON.

 \brief    Get DIP SW
*////////////////////////////////////////////////////////////////////////////////////////
UINT16 APPL_GetDipSw(void)
{
	UINT16 u16DipSw;

	u16DipSw = 0;

#if defined(BOARD_RA8T2_MCK)
	/* DIP SW type structure */
	sample_dip_sws_t dipsws = g_sample_dip_sws;
	bsp_io_level_t pin_value[4];

	/* This code uses BSP IO functions to show how it is used.*/
	R_BSP_PinAccessEnable();

	R_IOPORT_PinRead(g_ioport.p_ctrl, (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_0], &pin_value[SAMPLE_DIPSW_0]);
	R_IOPORT_PinRead(g_ioport.p_ctrl, (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_1], &pin_value[SAMPLE_DIPSW_1]);
	if (pin_value[SAMPLE_DIPSW_0] == BSP_IO_LEVEL_LOW) u16DipSw |= 0x01;
	if (pin_value[SAMPLE_DIPSW_1] == BSP_IO_LEVEL_LOW) u16DipSw |= 0x02;
	R_IOPORT_PinRead(g_ioport.p_ctrl, (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_2], &pin_value[SAMPLE_DIPSW_2]);
	if (pin_value[SAMPLE_DIPSW_2] == BSP_IO_LEVEL_LOW) u16DipSw |= 0x04;

	/* Protect PFS registers */
	R_BSP_PinAccessDisable();

#elif defined(BOARD_RA8T2_EK)
	/* get from I/O expander */
	(void)read_io_expander((UINT8 *)&u16DipSw);
#elif defined(BOARD_RA8T2_CPKNET)
    sample_dip_sws_t dipsws = g_sample_dip_sws;
    bsp_io_level_t pin_value[4];

    /* This code uses BSP IO functions to show how it is used.*/
    R_BSP_PinAccessEnable();

    R_IOPORT_PinRead(g_ioport.p_ctrl, (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_0], &pin_value[SAMPLE_DIPSW_0]);
    R_IOPORT_PinRead(g_ioport.p_ctrl, (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_1], &pin_value[SAMPLE_DIPSW_1]);
    if (pin_value[SAMPLE_DIPSW_0] == BSP_IO_LEVEL_LOW) u16DipSw |= 0x01;
    if (pin_value[SAMPLE_DIPSW_1] == BSP_IO_LEVEL_LOW) u16DipSw |= 0x02;
    R_IOPORT_PinRead(g_ioport.p_ctrl, (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_2], &pin_value[SAMPLE_DIPSW_2]);
    if (pin_value[SAMPLE_DIPSW_2] == BSP_IO_LEVEL_LOW) u16DipSw |= 0x04;
    /* Protect PFS registers */
    R_BSP_PinAccessDisable();

#endif // defined(BOARD_RA8T2_CPKNET)

	return u16DipSw;
}
#endif // defined(BOARD_RA8T2_MCK) | defined(BOARD_RA8T2_EK)
#endif // #if (CiA402_SAMPLE_APPLICATION == 1)

/** @} */
