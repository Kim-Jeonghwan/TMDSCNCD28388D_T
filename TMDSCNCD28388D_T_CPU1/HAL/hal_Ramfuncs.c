/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Ramfuncs.c
    Version          : 00.01
    Description      : RAM 빌드 전용 RamfuncsLoad/Run 더미 심볼 실제 정의 모듈
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비)
**********************************************************************/

/*
 * [목적]
 * RAM 빌드(_FLASH 미정의) 시 링크 단계에서 필요한 더미 심볼 실체를 제공합니다.
 */

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (작성자 기입 및 이력 블록 신설)
 */

/* ************************** [[   include  ]]  *********************************************************** */
#include "hal_Ramfuncs.h"

/* ************************** [[   global   ]]  *********************************************************** */
#ifndef _FLASH
/* RAM 빌드 전용: Ramfuncs Load/Run 더미 심볼 실제 정의 */
Uint16 RamfuncsLoadStart = 0U;
Uint16 RamfuncsLoadEnd   = 0U;
Uint16 RamfuncsLoadSize  = 0U;
Uint16 RamfuncsRunStart  = 0U;
Uint16 RamfuncsRunEnd    = 0U;
Uint16 RamfuncsRunSize   = 0U;
#endif /* #ifndef _FLASH */
