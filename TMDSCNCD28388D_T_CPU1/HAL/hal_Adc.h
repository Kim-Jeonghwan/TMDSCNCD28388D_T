/**********************************************************************
	Nexcom Co., Ltd.
	Filename         : hal_Adc.h
	Version          : 00.00
	Description      : CPU1 ADC 주변장치 드라이버 헤더 (온도센서 및 ePWM 트리거 설정)
	Programmer       : Kim Jeonghwan
	Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비 및 매크로 이동)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (작성자, 설명 기입, 매크로 및 전역 변수 이동)
 * 2026. 06. 02. - 온도 센서 전용 1kHz 느린 트리거용 ePWM9 함수 전역 선언 추가
 */


/* DESCRIPTION
 * 
 * 
 */

#ifndef HAL_ADC_H
#define HAL_ADC_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main_cpu1.h"

/* ************************** [[   define   ]]  *********************************************************** */
#define SYSCLK			200E6	// 200MHz, 28X Core(CPU) 시스템 클럭 주파수
#define TBCLK			 10E6	// 10MHz, EPWM 모듈 타임베이스 클럭 주파수
#define SAMPLING_FREQ	 10E3	// 10kHz, ADC 샘플링 주파수


#define BUFF_SIZE		500u     // ADC 결과 저장 버퍼 크기

#define CONV_ADC_3V		0.000732421875f		// 3 / 4096
#define CONV_ADC_3_3V	0.0008056640625f	// 3.3 / 4096

#define DEFAULT_MAVE_COUNT  100u   // 이동 평균 필터 카운트
#define DEFAULT_PWM_HZ      100000u // ePWM8 트리거 주파수 (100kHz 조정)



/* ************************** [[   struct   ]]  *********************************************************** */


/* ************************** [[   global   ]]  *********************************************************** */
extern uint16_t adcResult;


/* ************************** [[  function  ]]  *********************************************************** */
// ADC ISR 초기화
void InitialAdc(void);

// ADC 모듈 초기화 함수
void InitAdcModules(void);

void initEPWM8(void);
void initEPWM9(void); // 온도 센서 전용 1kHz 느린 트리거용 ePWM9 함수 추가

// ADCINA1 인터럽트 서비스 루틴 (뼈대)
interrupt void AdcaIsr(void);

#endif	// #ifndef HAL_ADC_H
