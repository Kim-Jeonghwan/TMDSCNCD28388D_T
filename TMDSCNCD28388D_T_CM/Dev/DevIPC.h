/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevIPC.h
    Description      : CM Core IPC Device Driver
    Last Updated     : 2026. 04. 21.
**********************************************************************/

#ifndef DEV_IPC_H
#define DEV_IPC_H

#include "main.h"

void Initial_IPC(void);
void sendIpcMessageToCPU1(uint32_t command, uint32_t addr, uint32_t data);

#endif // DEV_IPC_H
