/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevIPC.h
    Description      : CM Core IPC Device Driver Header
    Last Updated     : 2026. 06. 05. (코드 주석 포맷팅 및 한글화)
**********************************************************************/

#ifndef DEV_IPC_H
#define DEV_IPC_H

#include "main.h"

/* 전역 변수 */
extern volatile bool g_bCmReady;

/* 함수 프로토타입 */
void Initial_IPC_Clear(void);
void Initial_IPC_Mastership(void);
void Initial_IPC(void);
void sendEthDataToCM(uint16_t dspTemp, uint8_t seqNum, uint8_t status);


#endif // DEV_IPC_H
