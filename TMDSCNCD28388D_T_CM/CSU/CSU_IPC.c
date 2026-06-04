/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_IPC.c
    Description      : IPC Protocol (CM to CPU1) 구현
    Last Updated     : 2026. 06. 01. (IPC_CMD_CPU1_ETH_TX_DATA 수신 처리 추가)
**********************************************************************/

#include "CSU_IPC.h"

/* Message RAM 영역에 구조체 포인터 할당 */
volatile stIpcDataPacket *pxIpcCpu1ToCm = (volatile stIpcDataPacket *)IPC_CPU1_TO_CM_MSGRAM_ADDR;
volatile stIpcDataPacket *pxIpcCmToCpu1 = (volatile stIpcDataPacket *)IPC_CM_TO_CPU1_MSGRAM_ADDR;

/*
@funtion    void recvIpcCpu1Message(uint32_t command, uint32_t addr, uint32_t data)
@brief      CPU1 코어로부터 수신된 IPC 메시지 처리 핸들러
@param      uint32_t command: 수신된 IPC 명령어 코드
@param      uint32_t addr   : 수신된 데이터 (온도 x10 스케일 16bit = addr 하위 16비트)
@param      uint32_t data   : 수신된 보조 데이터 (SeqNum=하위 8bit, Status=다음 8bit)
@return     void
@remark
    - IPC_CMD_CPU1_ETH_TX_DATA: CPU1 → CM
        addr 하위 16bit = DspTemp (온도 x10)
        data 하위  8bit = SeqNum
        data   9~16bit = Status
*/
void recvIpcCpu1Message(uint32_t command, uint32_t addr, uint32_t data)
{
    if (command == IPC_CMD_CPU1_ETH_TX_DATA)
    {
        /* CPU1에서 보낸 온도 및 시퀀스 데이터 갱신 */
        g_xEthTxData.DspTemp = (uint16_t)(addr & 0x0000FFFFU);
        g_xEthTxData.SeqNum  = (uint8_t)(data & 0x000000FFU);
        g_xEthTxData.Status  = (uint8_t)((data >> 8U) & 0x000000FFU);
    }
    else
    {
        /* 정의되지 않은 IPC 명령 수신 시 예외 방어 */
    }
}

/*
@funtion    void processBulkDataFromCPU1(void)
@brief      공유 메모리(Message RAM)를 통한 대용량 데이터 처리 (현재 미사용 예비)
@param      void
@return     void
*/
void processBulkDataFromCPU1(void)
{
    uint32_t uiStatus = pxIpcCpu1ToCm->Status;

    if (uiStatus == 0x01U) /* Data Ready */
    {
        pxIpcCpu1ToCm->Status = 0x00U; /* Done */
    }
}
