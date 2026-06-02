/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevIPC.h
    Description      : CM Core IPC Device Driver Header
    Last Updated     : 2026. 06. 01. (sendEthDataToCM 프로토타입 추가)
**********************************************************************/

#ifndef DEV_IPC_H
#define DEV_IPC_H

#include "main.h"

/* 함수 프로토타입 */
void Initial_IPC_Mastership(void);
void Initial_IPC(void);
void sendEthDataToCM(uint16_t dspTemp, uint8_t seqNum, uint8_t status);


#endif // DEV_IPC_H
