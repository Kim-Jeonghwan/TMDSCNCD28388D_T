/**********************************************************************

	Nexcom Co., Ltd.
	Copyright 2021. All Rights Reserved.

	Filename		: hal_Timer.h
	Version			: 00.10
	Description		: 
	Tracebility		: 
	Programmer	    :
	Last Updated	: 2026. 05. 29.

**********************************************************************/

/*
 * 변경 이력
 * --------------------
 * 
 * 
 */


#ifndef HAL_TIMER_H
#define HAL_TIMER_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"


/* ************************** [[   define   ]]  *********************************************************** */


/* ************************** [[   enum or struct   ]]  *************************************************** */
typedef struct
{
	uint16_t Cycle_1ms;
	uint16_t Cycle_10ms;
	uint16_t Cycle_100ms;
	uint16_t Cycle_1000ms;

	uint16_t Hzcnt;
	uint16_t Hz;
} stTimer;


/* ************************** [[   global   ]]  *********************************************************** */
extern stTimer xTimer;


/* ************************** [[  function  ]]  *********************************************************** */
// DSP 타이머 초기화 
void Initial_TIMER(void);

__interrupt void isr_CpuTimer0(void);

__interrupt void isr_CpuTimer1(void);

__interrupt void isr_CpuTimer2(void);

#endif	// #ifndef HAL_TIMER_H



