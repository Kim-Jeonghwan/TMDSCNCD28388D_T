/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main_cpu1.h
    Version          : 00.02
    Description      : CPU1 전역 헤더 관리 파일
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (csu_Control.h 인클루드 추가)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - csu_Control.h 인클루드 추가 (CSU 계층 분리)
 * 2026. 06. 19. - 코드 스타일 및 주석 템플릿 적용
 * 2026. 06. 04. - easyDSP 호환성 유지: f28x_project.h 복원 / RamfuncsLoad* 심볼 문제는 hal_Ramfuncs.c로 해결
 */

#ifndef MAIN_CPU1_H
#define MAIN_CPU1_H

/* ************************** [[   include  ]]  *********************************************************** */
/* 표준 라이브러리 */
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

/* Driverlib 및 Device 기본 정의 */
// _DUAL_HEADERS가 선언되어 있어야 두 방식을 병행 가능합니다.
#include "driverlib.h"
#include "device.h"
#include "memcfg.h"
#ifndef MEMCFG_GSRAMMASTER_CM
#define MEMCFG_GSRAMMASTER_CM    2
#endif

/* Bit-field 헤더 포함 (구형 easyDSP 등 외부 코드와의 호환성 유지용)
// Uint16, Uint32 타입 및 비트필드 레지스터 구조체(AdcaRegs, GpioDataRegs 등) 제공 */
#include "f28x_project.h"

#include "hal_Common.h"
#include "hal_DspInit.h"
#include "hal_Sci.h"
#include "hal_Spi.h"
#include "hal_Timer.h"
#include "hal_Adc.h"
#include "hal_Ipc_cpu1.h"
#include "hal_Epwm.h"    /* EPWM1 기반 2ms 타이머 */
#include "hal_Ramfuncs.h"

#include "csu_SciPc.h"
#include "csu_Led.h"
#include "csu_Adc.h"
#include "csu_Epwm.h"
#include "csu_Ipc_cpu1.h"
#include "csu_Control.h"


/* ************************** [[   define   ]]  *********************************************************** */
//typedef uint8_t   Uint8; 



/* ************************** [[   enum or struct   ]]  *************************************************** */



/* ************************** [[   global   ]]  *********************************************************** */



/* ************************** [[  function  ]]  *********************************************************** */
// DSP program entry point
void main(void);


#endif	// #ifndef MAIN_CPU1_H
