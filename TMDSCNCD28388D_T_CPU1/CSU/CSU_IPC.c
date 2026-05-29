/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_IPC.c
    Description      : CM Core IPC 통신 프로토콜 구현
    Last Updated     : 2026. 05. 29. (정적 시험 기준 준수 및 보안 취약점 보완)
**********************************************************************/

#include "CSU_IPC.h"

stRcvIpcMsg xRcvIpcCmMsg;
stXmtIpcMsg xXmtIpcCmMsg;

// Message RAM 영역에 구조체 포인터 할당
volatile stIpcDataPacket *pxIpcCpu1ToCm = (volatile stIpcDataPacket *)IPC_CPU1_TO_CM_MSGRAM_ADDR;
volatile stIpcDataPacket *pxIpcCmToCpu1 = (volatile stIpcDataPacket *)IPC_CM_TO_CPU1_MSGRAM_ADDR;

/**
 * @brief CM으로부터 수신된 IPC 메시지 처리
 * @param command 수신된 명령
 * @param addr 수신된 주소 (필요 시 데이터 포인터)
 * @param data 수신된 데이터
 */
void recvIpcCmMessage(uint32_t command, uint32_t addr, uint32_t data)
{
    xRcvIpcCmMsg.Command = (uint16_t)(command & 0xFFFFu);
    xRcvIpcCmMsg.Address = addr;
    xRcvIpcCmMsg.Data = data;

    switch(xRcvIpcCmMsg.Command)
    {
        case 0x01u:
            // 예시 명령어 1 처리
            break;
            
        case 0x20u: // IPC_CMD_LOOPBACK_ON_RX (from CM)
            xLed.led01.State = true; 
            sendIpcMessageToCM(0x12, 0, 0); // IPC_ACK_LOOPBACK_ON
            sendSciPcMessage1();            // 즉시 PC 보고 (2ms 이내 응답 보장)
            break;
            
        case 0x21u: // IPC_CMD_LOOPBACK_OFF_RX
            xLed.led01.State = false;
            sendIpcMessageToCM(0x13, 0, 0); // IPC_ACK_LOOPBACK_OFF
            sendSciPcMessage1();            // 즉시 PC 보고
            break;
            
        case 0x22u: // IPC_ACK_LOOPBACK_ON_RX
            xLed.led02.State = true;
            sendSciPcMessage1();            // 즉시 PC 보고
            break;
            
        case 0x23u: // IPC_ACK_LOOPBACK_OFF_RX
            xLed.led02.State = false;
            sendSciPcMessage1();            // 즉시 PC 보고
            break;
            
        default:
            break;
    }
}

/**
 * @brief CM으로 보낼 데이터 전송
 */
void sendIpcCmMessage1(void)
{
    uint32_t sendCmd = xXmtIpcCmMsg.Command;
    uint32_t sendAddr = xXmtIpcCmMsg.Address;
    uint32_t sendData = xXmtIpcCmMsg.Data;
    
    sendIpcMessageToCM(sendCmd, sendAddr, sendData);
}
/**
 * @brief CM으로 대용량 데이터 전송 예시
 */
void sendBulkDataToCM(uint32_t cmd, uint32_t *data, uint16_t length)
{
    uint16_t i;
    
    if (data != NULL)
    {
        // 1. Message RAM 구조체에 데이터 채우기
        pxIpcCpu1ToCm->Command = cmd;
        pxIpcCpu1ToCm->Status = 0x01; // Busy or Data Ready
        
        for(i = 0; i < length && i < 16; i++)
        {
            pxIpcCpu1ToCm->Payload[i] = data[i];
        }
        
        // 2. IPC Flag를 Set하여 CM에게 데이터가 준비되었음을 알림 (예: IPC_FLAG0)
        IPC_setFlagLtoR(IPC_CPU1_L_CM_R, IPC_FLAG0);
    }
}
