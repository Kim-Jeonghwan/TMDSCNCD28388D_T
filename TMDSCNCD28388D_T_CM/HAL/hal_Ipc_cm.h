/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Ipc_cm.h
    Version          : 00.00
    Description      : CM Core IPC Device Driver
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (모듈 및 파일명 리팩토링)
**********************************************************************/

#ifndef HAL_IPC_CM_H
#define HAL_IPC_CM_H

#include "main_cm.h"

void Initial_IPC(void);
void sendIpcMessageToCPU1(uint32_t command, uint32_t addr, uint32_t data);

#endif // HAL_IPC_CM_H
