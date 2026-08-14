/*
* This source file is part of the EtherCAT Slave Stack Code licensed by Beckhoff Automation GmbH & Co KG, 33415 Verl, Germany.
* The corresponding license agreement applies. This hint shall not be removed.
*/

/**
 * \addtogroup SampleAppl Sample Application
 * @{
 */

/**
\file sampleappl.h
\author EthercatSSC@beckhoff.com
\brief Sample application specific objects

\version 5.12

<br>Changes to version V5.11:<br>
V5.12 EOE4: handle 16bit only acceess, move ethernet protocol defines and structures to application header files<br>
<br>Changes to version V5.01:<br>
V5.11 COE1: update invalid end entry in the object dictionaries (error with some compilers)<br>
V5.11 ECAT4: enhance SM/Sync monitoring for input/output only slaves<br>
<br>Changes to version - :<br>
V5.01 : Start file change log
 */

/*-----------------------------------------------------------------------------------------
------
------    Includes
------
-----------------------------------------------------------------------------------------*/
#include "cia402appl.h"
#include "ecat_def.h"

#if (CiA402_SAMPLE_APPLICATION == 1)

#ifndef _CIA402_SAMPLE_H_
#define _CIA402_SAMPLE_H_

/*-----------------------------------------------------------------------------------------
------
------    Defines and Types
------
-----------------------------------------------------------------------------------------*/

#endif // #ifndef _CIA402_SAMPLE_H_

void DummyMotor(void);
void CiA402_DummyMotionControl(TCiA402Axis *pCiA402Axis, UINT16 i);
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

UINT16 CiA402_Init(void);
void CiA402_DeallocateAxis(void);
BOOL CiA402_TransitionAction(INT16 Characteristic,TCiA402Axis *pCiA402Axis);
void CiA402_Application(TCiA402Axis *pCiA402Axis, UINT16 i);

UINT16 APPL_GetDeviceID(void);
UINT16 APPL_GetDipSw(void);
void APPL_SetLed(UINT16 value);

#endif // #if (CiA402_SAMPLE_APPLICATION == 1)
