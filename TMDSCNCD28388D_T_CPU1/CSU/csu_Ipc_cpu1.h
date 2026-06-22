/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Ipc_cpu1.h
    Version          : 00.04
    Description      : CPU1 IPC 및 공유 메모리 통신 프로토콜 정의
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 22. (GSRAM 하드웨어 접근 불가로 인해 MSGRAM으로 롤백 및 최적화)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 22. - GSRAM 기반 공유 메모리(GS0, GS1) 사용으로 전환
 * 2026. 06. 22. - Lock-Free 동기화를 위한 stIpcDataPacket에 seqCount 추가
 * 2026. 06. 22. - CM 코어의 GSRAM 쓰기 권한 부재(Hard Fault 방지)로 인해 MSGRAM으로 맵핑 변경
 */

#ifndef CSU_IPC_CPU1_H
#define CSU_IPC_CPU1_H

#include "main_cpu1.h"

// IPC 통신용 공용 데이터 구조체 (32비트 정렬 권장)
typedef struct {
    uint32_t seqCount;      // 동기화용 Seqlock 카운터 (짝수=완료, 홀수=쓰기중)
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
        struct {
            uint32_t seqNum;
            uint32_t status;
        } RxData;
    } Payload;
} stIpcDataPacket;

// Message RAM 정의 (F2838x 하드웨어 매핑)
// CPU1 -> CM: 0x39000 (CPU1 View) == 0x20080000 (CM View)
// CM -> CPU1: 0x38000 (CPU1 View) == 0x20082000 (CM View)
#define IPC_CPU1_TO_CM_MSGRAM_ADDR    0x39000U
#define IPC_CM_TO_CPU1_MSGRAM_ADDR    0x38000U

/* IPC 명령어 정의 */
#define IPC_CMD_CPU1_ETH_TX_DATA  (0x2001U)  /* CPU1 -> CM: 온도+시퀀스 전달 */
#define IPC_CMD_CM_ETH_RX_DATA    (0x2002U)  /* CM -> CPU1: 수신 SeqNum/Status 전달 */
#define IPC_CMD_CM_BOOT_READY     (0x3001U)  /* CM -> CPU1: CM 기동 및 주변기기 초기화 완료 */

extern volatile stIpcDataPacket *pxDataCpu1ToCm;
extern volatile stIpcDataPacket *pxDataCmToCpu1;

/* CM 코어로부터 이더넷 수신 데이터 구조체 */
typedef struct {
    uint8_t seqNum;
    uint8_t status;
} stEthRxData;

extern volatile stEthRxData xEthRxData;

/* CM에서 수신한 IPC 메시지 체인 함수 */
void recvIpcCmMessage(uint32_t command, uint32_t addr, uint32_t data);

#endif // CSU_IPC_CPU1_H
