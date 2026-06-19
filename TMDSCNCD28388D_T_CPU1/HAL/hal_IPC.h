/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_IPC.h
    Description      : CM Core IPC Device Driver Header
    Last Updated     : 2026. 06. 05. (코드 주석 포맷팅 및 한글화)
**********************************************************************/

#ifndef HAL_IPC_H
#define HAL_IPC_H

#include "main.h"

/* 전역 변수 */
extern volatile bool g_bCmReady;

/* 함수 프로토타입 */
void Initial_IPC_Clear(void);
void Initial_IPC_Mastership(void);
void Initial_IPC(void);
#endif // HAL_IPC_H
