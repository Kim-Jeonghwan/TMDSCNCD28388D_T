/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Ipc_cpu1.h
    Version          : 00.02
    Description      : CPU1 IPC 통신 프로토콜 정의
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (모듈 및 파일명 리팩토링)
**********************************************************************/

#ifndef CSU_IPC_CPU1_H
#define CSU_IPC_CPU1_H

#include "main_cpu1.h"

// IPC 통신용 공용 데이터 구조체 (32비트 정렬 권장)
typedef struct {
    uint32_t Command;       // 명령어
    uint32_t Status;        // 상태 플래그
    uint32_t Address;       // 메모리 주소 (필요시)
    union {
        uint32_t PayloadRaw[16];   // 실제 데이터 배열
        struct {
            float32_t sineValue;
            float32_t adcTemperature;
            uint32_t sequenceNum;
        } TxData;
    } Payload;
} stIpcDataPacket;

// Message RAM 정의 (F2838x 하드웨어 매핑)
// CPU1 -> CM: 0x39000 (CPU1 View) == 0x20080000 (CM View)
// CM -> CPU1: 0x38000 (CPU1 View) == 0x20082000 (CM View)
#define IPC_CPU1_TO_CM_MSGRAM_ADDR    0x39000U
#define IPC_CM_TO_CPU1_MSGRAM_ADDR    0x38000U

// 1. GSRAM 주소 정의 (F2838x 정적 매핑)
#define GS0_CPU_START_ADDR    0x0000D000U  // C28x GS0 Start
#define GS0_CM_START_ADDR     0x20014000U  // CM GS0 Start
#define GS1_CPU_START_ADDR    0x0000E000U  // C28x GS1 Start
#define GS1_CM_START_ADDR     0x20016000U  // CM GS1 Start

// 2. 주소 변환 매크로 (CM Byte Addr <-> CPU Word Addr)
#define CONVERT_CM_TO_CPU_ADDR(cm_addr)  (GS0_CPU_START_ADDR + (((cm_addr) - GS0_CM_START_ADDR) >> 1))
#define CONVERT_CPU_TO_CM_ADDR(cpu_addr) (GS0_CM_START_ADDR + (((cpu_addr) - GS0_CPU_START_ADDR) << 1))

/* IPC 명령어 정의 */
#define IPC_CMD_CPU1_ETH_TX_DATA  (0x2001U)  /* CPU1 -> CM: 온도+시퀀스 전달 */
#define IPC_CMD_CM_ETH_RX_DATA    (0x2002U)  /* CM -> CPU1: 수신 SeqNum/Status 전달 */
#define IPC_CMD_CM_BOOT_READY     (0x3001U)  /* CM -> CPU1: CM 기동 및 주변기기 초기화 완료 */

extern volatile stIpcDataPacket *pxIpcCpu1ToCm;
extern volatile stIpcDataPacket *pxIpcCmToCpu1;

/* CM 코어로부터 이더넷 수신 데이터 구조체 */
typedef struct {
    uint8_t seqNum;
    uint8_t status;
} stEthRxData;

extern volatile stEthRxData xEthRxData;

/* CM에서 수신한 IPC 메시지 체인 함수 */
void recvIpcCmMessage(uint32_t command, uint32_t addr, uint32_t data);

#endif // CSU_IPC_CPU1_H
