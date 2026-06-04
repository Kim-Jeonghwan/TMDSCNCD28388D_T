/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main.h
    Description      : Core header files inclusion
    Last Updated     : 2026. 06. 04. (easyDSP 호환성 유지: f28x_project.h 복원 / RamfuncsLoad* 심볼 문제는 DevRamfuncs.c로 해결)
**********************************************************************/

#ifndef MAIN_H
#define MAIN_H

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

#include "DevCommon.h"
#include "DevDspInit.h"
#include "DevSci.h"
#include "DevSpi.h"
#include "DevTimer.h"
#include "DevAdc.h"
#include "DevIPC.h"
#include "DevEpwmTimer.h"    /* EPWM1 기반 2ms 타이머 */
#include "DevRamfuncs.h"

#include "CSU_SCI_PC.h"
#include "CSU_LED.h"
#include "CSU_Adc.h"
#include "CSU_EPWM.h"
#include "CSU_IPC.h"


/* ************************** [[   define   ]]  *********************************************************** */
//typedef uint8_t   Uint8; 



/* ************************** [[   enum or struct   ]]  *************************************************** */



/* ************************** [[   global   ]]  *********************************************************** */



/* ************************** [[  function  ]]  *********************************************************** */
// DSP program entry point
void main(void);


#endif	// #ifndef MAIN_H

