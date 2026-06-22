/**********************************************************************
	Nexcom Co., Ltd.
	Filename         : hal_Timer.h
	Version          : 00.10
	Description      : CPU1 시스템 주기 타이머 (CPUTimer 0, 1, 2) 드라이버 헤더
	Programmer       : Kim Jeonghwan
	Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (작성자 기입 및 이력 블록 보완)
 */


#ifndef HAL_TIMER_H
#define HAL_TIMER_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main_cpu1.h"


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



