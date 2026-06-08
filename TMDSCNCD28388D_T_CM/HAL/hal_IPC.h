/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_IPC.h
    Description      : CM Core IPC Device Driver
    Last Updated     : 2026. 06. 05. (코드 주석 포맷팅 및 한글화)
**********************************************************************/

#ifndef HAL_IPC_H
#define HAL_IPC_H

#include "main.h"

void Initial_IPC(void);
void sendIpcMessageToCPU1(uint32_t command, uint32_t addr, uint32_t data);

#endif // HAL_IPC_H
