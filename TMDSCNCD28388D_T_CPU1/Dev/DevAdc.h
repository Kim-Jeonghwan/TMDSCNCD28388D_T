/**********************************************************************

	Nexcom Co., Ltd.
	Copyright 2021. All Rights Reserved.

	Filename		: DevAdc.h
	Version			: 00.00
	Description		: 
	Tracebility		: 
	Programmer		: 
	Last Updated	: 2026. 04. 13.

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
extern float32_t f32PotenRAW;
extern float32_t f32PotenMAVE;
extern float32_t f32PWMRaw;
extern float32_t f32PWMRCLPF;
extern float32_t f32PWMBWLPF;

/* ************************** [[  function  ]]  *********************************************************** */
// ADC 인터럽트 서비스 루틴(함수)
interrupt void AdcaIsr(void);
//interrupt void AdcbIsr(void);
//interrupt void AdccIsr(void);
//interrupt void AdcdIsr(void);

// ADC ISR 초기화
void InitialAdc(void);

// ADC 모듈 초기화 함수
void InitAdcModules(void);

float32_t low_pass_filter(float32_t input, float32_t alpha, float32_t *prev_output);


float32_t Within_f32(float32_t val, float32_t min, float32_t max);

void initEPWM8(void);

#endif	// #ifndef DEVADC_H
