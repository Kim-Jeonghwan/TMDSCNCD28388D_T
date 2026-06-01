/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main.h
    Description      : Core header files inclusion
    Last Updated     : 2026. 06. 01.
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

/* Bit-field 헤더 포함 */
// f28x_project.h 내부의 Uint16 등과 충돌을 피하기 위해 
// 프로젝트 전체에서 사용하는 타입을 여기서 정리합니다.
#include "f28x_project.h"

#include "DevCommon.h"
#include "DevDspInit.h"
#include "DevSci.h"
#include "DevSpi.h"
#include "DevTimer.h"
#include "DevAdc.h"
#include "DevIPC.h"

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

