/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_IPC.c
    Description      : CM Core IPC 통신 프로토콜 구현
    Last Updated     : 2026. 06. 01. (정적 시험 기준 준수 및 보안 취약점 보완)
**********************************************************************/

#include "CSU_IPC.h"



// Message RAM 영역에 구조체 포인터 할당
volatile stIpcDataPacket *pxIpcCpu1ToCm = (volatile stIpcDataPacket *)IPC_CPU1_TO_CM_MSGRAM_ADDR;
volatile stIpcDataPacket *pxIpcCmToCpu1 = (volatile stIpcDataPacket *)IPC_CM_TO_CPU1_MSGRAM_ADDR;


