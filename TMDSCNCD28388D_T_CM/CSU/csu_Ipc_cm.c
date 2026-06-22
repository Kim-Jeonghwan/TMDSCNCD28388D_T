/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Ipc_cm.c
    Version          : 00.07
    Description      : CM IPC Protocol 및 공유 메모리 구현
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 22. (GSRAM 잔재 주석을 MSGRAM 기준으로 수정)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 22. - GSRAM 잔재 주석을 MSGRAM 기준으로 수정
 * 2026. 06. 22. - pxDataCpu1ToCm, pxDataCmToCpu1 포인터를 MSGRAM 주소로 맵핑
 * 2026. 06. 22. - IPC 전송 기반이 폴링으로 변경됨에 따라 recvIpcCpu1Message, processBulkDataFromCPU1 제거
 * 2026. 06. 22. - F2838x 하드웨어 설계상 CM 코어는 GSRAM에 대한 쓰기 권한이 없어 MSGRAM 주소로 원복
 */

#include "csu_Ipc_cm.h"

/* MSGRAM 영역에 구조체 포인터 할당 */
volatile stIpcDataPacket *pxDataCpu1ToCm = (volatile stIpcDataPacket *)IPC_CPU1_TO_CM_MSGRAM_ADDR;
volatile stIpcDataPacket *pxDataCmToCpu1 = (volatile stIpcDataPacket *)IPC_CM_TO_CPU1_MSGRAM_ADDR;

