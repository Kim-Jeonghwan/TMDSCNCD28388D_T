/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_IPC.c
    Description      : CM Core IPC 통신 프로토콜 구현
    Last Updated     : 2026. 06. 05. (코드 주석 포맷팅 및 한글화)
**********************************************************************/

#include "CSU_IPC.h"

/* Message RAM 영역에 구조체 포인터 할당 */
volatile stIpcDataPacket *pxIpcCpu1ToCm = (volatile stIpcDataPacket *)IPC_CPU1_TO_CM_MSGRAM_ADDR;
volatile stIpcDataPacket *pxIpcCmToCpu1 = (volatile stIpcDataPacket *)IPC_CM_TO_CPU1_MSGRAM_ADDR;

/* CM→CPU1 수신 공유 변수 (CSU_SCI_PC.c 등에서 참조) */
uint8_t  g_ucEthRxSeqNum  = 0U;
uint8_t  g_ucEthRxStatus  = 0U;

/*
@funtion    void recvIpcCmMessage(uint32_t command, uint32_t addr, uint32_t data)
@brief      CM 코어로부터 수신된 IPC 메시지 처리 (CPU1 측)
@param      uint32_t command: IPC 명령어 코드
@param      uint32_t addr   : 보조 데이터 addr (미사용)
@param      uint32_t data   : 하위 8bit = SeqNum, 9~16bit = Status
@return     void
@remark
    - IPC_CMD_CM_ETH_RX_DATA: CM이 PC로부터 수신한 Update MSG 데이터를 CPU1으로 전달
*/
void recvIpcCmMessage(uint32_t command, uint32_t addr, uint32_t data)
{
    (void)addr; /* 미사용 파라미터 */

    if (command == IPC_CMD_CM_ETH_RX_DATA)
    {
        g_ucEthRxSeqNum = (uint8_t)(data & 0x000000FFU);
        g_ucEthRxStatus = (uint8_t)((data >> 8U) & 0x000000FFU);
    }
    else
    {
        /* 정의되지 않은 명령어 수신 시 예외 방어 */
    }
}
