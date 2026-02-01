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
#include "cia402sample.h"

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

TAxis LocalAxis[MAX_AXES];
//static void DummyMotor(uint32_t channel);

UINT16 CiA402_StateTransition1(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition2(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition3(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition4(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition5(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition6(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition7(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition8(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition9(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition10(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition11(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition12(TCiA402Axis *pCiA402Axis);
void CiA402_LocalError(UINT16 ErrorCode);
UINT16 CiA402_StateTransition14(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition15(TCiA402Axis *pCiA402Axis);
UINT16 CiA402_StateTransition16(TCiA402Axis *pCiA402Axis);
UINT16 APPL_MOTOR_MotionControl_Main(TCiA402Axis *pCiA402Axis, UINT16 i);


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
/*******************************************************************************
* Function Name: DummyMotor
* Description  : Increment VelocityActualValue
* Arguments    : channel
* Return Value : none
*******************************************************************************/
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
				if(LocalAxis[i].PositionActualValue < LocalAxis[i].TargetPosition)
				{
					LocalAxis[i].PositionActualValue++;
				}
			}
			else
			{
				LocalAxis[i].PositionActualValue = 0;
			}
			break;
		case CYCLIC_SYNC_VELOCITY_MODE:
			if(pCiA402Axis->i16State == STATE_OPERATION_ENABLED)
			{
				if(LocalAxis[i].VelocityActualValue < LocalAxis[i].TargetVelocity)
				{
					LocalAxis[i].VelocityActualValue++;
				}
			}
			else
			{
				LocalAxis[i].VelocityActualValue = 0;
			}
			break;
		default:
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \brief    CiA402_DummyMotionControl
 \brief this functions provides an simple feedback functionality
*////////////////////////////////////////////////////////////////////////////////////////
void CiA402_DummyMotionControl(TCiA402Axis *pCiA402Axis, UINT16 i)
{
	/************* DUMMY MOTION CONTROL ****************/

	/* update actual value's */
	pCiA402Axis->Objects.objPositionActualValue = LocalAxis[i].PositionActualValue;
	pCiA402Axis->Objects.objVelocityActualValue = LocalAxis[i].VelocityActualValue;

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
					pCiA402Axis->Objects.objStatusWord &= ~STATUSWORD_INTERNAL_LIMIT;
					/* execute position control */
					LocalAxis[i].TargetPosition = pCiA402Axis->Objects.objTargetPosition;
				}else{
					/* set internal limit flag */
					pCiA402Axis->Objects.objStatusWord |= STATUSWORD_INTERNAL_LIMIT;
				}
			break;

			/* velocity mode */
			case CYCLIC_SYNC_VELOCITY_MODE:
				/* execute velocity control */
				LocalAxis[i].TargetVelocity = pCiA402Axis->Objects.objTargetVelocity;
			/* torque mode */
			case CYCLIC_SYNC_TORQUE_MODE:
			/* other */
			default:
				/* do nothing (not supported) */
			break;
		}
	}

	/* Accept new mode of operation */
	pCiA402Axis->Objects.objModesOfOperationDisplay = pCiA402Axis->Objects.objModesOfOperation;
}

UINT16 CiA402_StateTransition1(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition1 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition2(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition2 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition3(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition3 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition4(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition4 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition5(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition5 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition6(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition6 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition7(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition7 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition8(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition8 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition9(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition9 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition10(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition10 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition11(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition11 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition12(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition12 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \param ErrorCode

 \brief    CiA402_LocalError
 \brief this function is called if an error was detected
*////////////////////////////////////////////////////////////////////////////////////////
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

UINT16 CiA402_StateTransition14(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition14 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition15(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition15 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 CiA402_StateTransition16(TCiA402Axis *pCiA402Axis)
{
#if(_DUMMY_ == 1)
	printf("Axis%d :StateTransition16 Pass\n",pCiA402Axis->u16AxisNum);
#endif
	return 0;
}

UINT16 APPL_MOTOR_MotionControl_Main(TCiA402Axis *pCiA402Axis, UINT16 i)
{
	CiA402_DummyMotionControl(pCiA402Axis, i);
	return 0;
}

/** @} */

