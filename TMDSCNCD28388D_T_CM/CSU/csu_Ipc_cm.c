/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Ipc_cm.c
    Version          : 00.02
    Description      : CM IPC Protocol 구현
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (모듈 및 파일명 리팩토링)
**********************************************************************/

#include "csu_Ipc_cm.h"

/* Message RAM 영역에 구조체 포인터 할당 */
volatile stIpcDataPacket *pxIpcCpu1ToCm = (volatile stIpcDataPacket *)IPC_CPU1_TO_CM_MSGRAM_ADDR;
volatile stIpcDataPacket *pxIpcCmToCpu1 = (volatile stIpcDataPacket *)IPC_CM_TO_CPU1_MSGRAM_ADDR;

/*
@function    recvIpcCpu1Message
@brief      CPU1 코어로부터 수신된 IPC 메시지 처리 핸들러
@param      uint32_t command: 수신된 IPC 명령어 코드
@param      uint32_t addr   : 수신된 데이터 (미사용)
@param      uint32_t data   : 수신된 보조 데이터 (미사용)
@return     void
@remark
    - IPC_CMD_CPU1_ETH_TX_DATA: CPU1 → CM
      pxIpcCpu1ToCm->Payload.TxData 파싱
*/
void recvIpcCpu1Message(uint32_t command, uint32_t addr, uint32_t data)
{
    (void)addr;
    (void)data;

    if (command == IPC_CMD_CPU1_ETH_TX_DATA)
    {
        /* CPU1에서 보낸 온도, 시퀀스, 사인파 데이터 갱신 */
        g_xEthTxData.SineVal = pxIpcCpu1ToCm->Payload.TxData.sineValue;
        g_xEthTxData.DspTemp = (uint16_t)pxIpcCpu1ToCm->Payload.TxData.adcTemperature;
        g_xEthTxData.SeqNum  = (uint8_t)(pxIpcCpu1ToCm->Payload.TxData.sequenceNum & 0xFFU);
        g_xEthTxData.Status  = 0U; /* TODO: CPU1에서 상태 필드 제거됨에 따른 기본값 */
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
