/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Ipc_cpu1.h
    Version          : 00.01
    Description      : CM Core IPC Device Driver Header
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (이력 블록 신설)
 * 2026. 06. 22. - MSGRAM 롤백으로 인해 불필요해진 Initial_IPC_Mastership 삭제
 */

#ifndef HAL_IPC_CPU1_H
#define HAL_IPC_CPU1_H

#include "main_cpu1.h"

/* 구조체 및 전역 변수 */
typedef struct {
    bool isCmReady;
} stIpcState;

extern volatile stIpcState xIpcState;

/* 함수 프로토타입 */
void Initial_IPC_Clear(void);

void Initial_IPC(void);
#endif // HAL_IPC_CPU1_H
