/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Ipc_cpu1.c
    Version          : 00.03
    Description      : CM Core IPC 통신 프로토콜 및 공유 메모리 구현
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 22. (GSRAM 잔재 주석을 MSGRAM 기준으로 수정)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 22. - GSRAM 잔재 주석을 MSGRAM 기준으로 수정
 * 2026. 06. 22. - pxDataCpu1ToCm, pxDataCmToCpu1 포인터를 MSGRAM 주소로 맵핑
 * 2026. 06. 22. - 불필요해진 recvIpcCmMessage() 제거
 * 2026. 06. 22. - CM 코어의 GSRAM 쓰기 권한 부재(Hard Fault 방지)로 인해 MSGRAM 주소로 원복
 */

#include "csu_Ipc_cpu1.h"

/* MSGRAM 영역에 구조체 포인터 할당 */
volatile stIpcDataPacket *pxDataCpu1ToCm = (volatile stIpcDataPacket *)IPC_CPU1_TO_CM_MSGRAM_ADDR;
volatile stIpcDataPacket *pxDataCmToCpu1 = (volatile stIpcDataPacket *)IPC_CM_TO_CPU1_MSGRAM_ADDR;

/* CM→CPU1 수신 공유 변수 (csu_SciPc.c 등에서 참조) */
volatile stEthRxData xEthRxData = {0U, 0U};

