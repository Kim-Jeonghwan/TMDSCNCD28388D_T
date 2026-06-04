/**********************************************************************
    Nexcom Co., Ltd.
    Copyright 2021. All Rights Reserved.

    Filename        : DevRamfuncs.c
    Version         : 00.01
    Description     : RAM 빌드 전용 RamfuncsLoad/Run 더미 심볼 실제 정의 모듈
    Tracebility     :
    Programmer      : 
    Last Updated    : 2026. 06. 04.

    [목적]
    RAM 빌드(_FLASH 미정의) 시 링크 단계에서 필요한 더미 심볼 실체를 제공합니다.
**********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "DevRamfuncs.h"

/* ************************** [[   global   ]]  *********************************************************** */
#ifndef _FLASH
/* RAM 빌드 전용: Ramfuncs Load/Run 더미 심볼 실제 정의 */
uint16_t RamfuncsLoadStart = 0U;
uint16_t RamfuncsLoadEnd   = 0U;
uint16_t RamfuncsLoadSize  = 0U;
uint16_t RamfuncsRunStart  = 0U;
uint16_t RamfuncsRunEnd    = 0U;
uint16_t RamfuncsRunSize   = 0U;
#endif /* #ifndef _FLASH */
