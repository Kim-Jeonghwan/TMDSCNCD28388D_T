/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_Tact.h
    Description      : Tact Switch Input Control (GPIO 64, 65)
    Last Updated     : 2026. 04. 17.
**********************************************************************/

#ifndef CSU_TACT_H
#define CSU_TACT_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"


/* ************************** [[   define   ]]  *********************************************************** */
#define GPIO_TACT_01    64u
#define GPIO_TACT_02    65u


/* ************************** [[  function  ]]  *********************************************************** */

/**
 * @brief Tact 스위치 제어를 위한 GPIO 방향 및 Mux 설정
 */
void initGpioDinTact(void);

/**
 * @brief Tact 스위치 변수 초기화
 */
void Initial_Tact(void);

/**
 * @brief Tact 스위치 상태 업데이트 (GPIO 64, 65 -> xXmtSciPcMsg1)
 */
void updateTactStatus(void);

#endif	// #ifndef CSU_TACT_H
