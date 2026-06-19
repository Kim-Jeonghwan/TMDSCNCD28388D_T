/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Ipc_cpu1.h
    Version          : 00.00
    Description      : CM Core IPC Device Driver Header
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (모듈 및 파일명 리팩토링)
**********************************************************************/

#ifndef HAL_IPC_CPU1_H
#define HAL_IPC_CPU1_H

#include "main_cpu1.h"

/* 전역 변수 */
extern volatile bool g_bCmReady;

/* 함수 프로토타입 */
void Initial_IPC_Clear(void);
void Initial_IPC_Mastership(void);
void Initial_IPC(void);
#endif // HAL_IPC_CPU1_H
