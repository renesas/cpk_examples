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
#include "ecat_def.h"

#ifndef _CIA402_SAMPLE_H_
#define _CIA402_SAMPLE_H_



/*-----------------------------------------------------------------------------------------
------
------    Defines and Types
------
-----------------------------------------------------------------------------------------*/
extern void DummyMotor(void);
/*ECATCHANGE_START(V5.12) EOE4*/
extern UINT16 APPL_GetDeviceID();
#endif/** @}*/
