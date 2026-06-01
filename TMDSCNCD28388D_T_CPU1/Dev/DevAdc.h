/**********************************************************************

	Nexcom Co., Ltd.
	Copyright 2021. All Rights Reserved.

	Filename		: DevAdc.h
	Version			: 00.00
	Description		: 
	Tracebility		: 
	Programmer		: 
	Last Updated	: 2026. 06. 01. (ADC ISR 및 updateAdcData 뼈대 복원)

	Function List	:	
						

**********************************************************************/

/*
 * Modification History
 * --------------------
 * 
 * 
*/


/* DESCRIPTION
 * 
 * 
*/

#ifndef DEVADC_H
#define DEVADC_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"

/* ************************** [[   define   ]]  *********************************************************** */
#define SYSCLK			200E6	// 200MHz, 28X Core(CPU) 시스템 클럭 주파수
#define TBCLK			 10E6	// 10MHz, EPWM 모듈 타임베이스 클럭 주파수
#define SAMPLING_FREQ	 10E3	// 10kHz, ADC 샘플링 주파수


#define BUFF_SIZE		500u     // ADC 결과 저장 버퍼 크기

#define CONV_ADC_3V		0.000732421875f		// 3 / 4096
#define CONV_ADC_3_3V	0.0008056640625f	// 3.3 / 4096



/* ************************** [[   struct   ]]  *********************************************************** */


/* ************************** [[   global   ]]  *********************************************************** */


/* ************************** [[  function  ]]  *********************************************************** */
// ADC ISR 초기화
void InitialAdc(void);

// ADC 모듈 초기화 함수
void InitAdcModules(void);

void initEPWM8(void);

// ADCINA1 인터럽트 서비스 루틴 (뼈대)
interrupt void AdcaIsr(void);

#endif	// #ifndef DEVADC_H
