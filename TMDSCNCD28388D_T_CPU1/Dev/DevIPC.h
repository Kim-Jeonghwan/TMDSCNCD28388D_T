/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevIPC.h
    Description      : CM Core IPC Device Driver Header
    Last Updated     : 2026. 05. 06.
**********************************************************************/

#ifndef DEV_IPC_H
#define DEV_IPC_H

#include "main.h"

// Function Prototypes
void Initial_IPC_Mastership(void);
void Initial_IPC(void);
void sendIpcMessageToCM(uint32_t command, uint32_t addr, uint32_t data);

#endif // DEV_IPC_H
