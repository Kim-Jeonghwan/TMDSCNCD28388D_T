/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Ipc_cm.h
    Version          : 00.06
    Description      : CM IPC 및 공유 메모리 통신 프로토콜 정의
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 22. (GSRAM 잔재 주석을 MSGRAM 기준으로 수정)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 22. - GSRAM 잔재 주석을 MSGRAM 기준으로 수정
 * 2026. 06. 22. - 파형 선택 기능 지원을 위해 구조체 내부 필드명 변경 (status -> waveType, sineValue -> waveValue)
 * 2026. 06. 22. - MSGRAM 기반 공유 메모리 사용으로 stIpcDataPacket에 seqCount 추가
 * 2026. 06. 22. - 포인터 변수명을 pxDataCpu1ToCm / pxDataCmToCpu1 로 변경
 * 2026. 06. 22. - CM 코어는 GSRAM에 쓰기 권한이 없으므로(Hard Fault 발생), 통신 수단을 MSGRAM으로 원복
 */

#ifndef CSU_IPC_CM_H
#define CSU_IPC_CM_H

#include "main_cm.h"

// IPC 통신용 공용 데이터 구조체 (32비트 정렬)
typedef struct {
    uint32_t seqCount;      // 동기화용 Seqlock 카운터 (짝수=완료, 홀수=쓰기중)
    uint32_t Command;       // 명령어
    uint32_t Status;        // 상태 플래그
    uint32_t Address;       // 메모리 주소
    union {
        uint32_t PayloadRaw[16];   // 실제 데이터 배열
        struct {
            float32_t waveValue;
            float32_t adcTemperature;
            uint32_t sequenceNum;
        } TxData;
        struct {
            uint32_t seqNum;
            uint32_t waveType;
        } RxData;
    } Payload;
} stIpcDataPacket;

// Message RAM 정의 (CM View)
#define IPC_CPU1_TO_CM_MSGRAM_ADDR    0x20080000U
#define IPC_CM_TO_CPU1_MSGRAM_ADDR    0x20082000U

// --- 이더넷 패킷 전용 공유 메모리 설정 ---
// 1. 이더넷 패킷 데이터 구조 (MSGRAM에 배치될 데이터)
typedef struct {
    uint32_t Length;        // 패킷 길이 (Bytes)
    uint32_t Reserved;      // 64비트 정렬용
    uint8_t  Data[1514];    // 실제 이더넷 프레임 데이터 (MTU 기준)
} stEthPacketBuffer;

// IPC 명령어 정의
#define IPC_CMD_ETH_RCV_NOTIFY    0x1001U  /* CM -> CPU1: 패킷 수신 알림 */
#define IPC_CMD_ETH_XMT_REQUEST   0x1002U  /* CPU1 -> CM: 패킷 송신 요청 */
#define IPC_CMD_CPU1_ETH_TX_DATA  0x2001U  /* CPU1 -> CM: 온도+시퀀스 전달 */
#define IPC_CMD_CM_ETH_RX_DATA    0x2002U  /* CM -> CPU1: 수신 SeqNum/Cmd 전달 */
#define IPC_CMD_CM_BOOT_READY     0x3001U  /* CM -> CPU1: CM 기동 및 주변기기 초기화 완료 */

extern volatile stIpcDataPacket *pxDataCpu1ToCm;
extern volatile stIpcDataPacket *pxDataCmToCpu1;

// 제거됨: void recvIpcCpu1Message(uint32_t command, uint32_t addr, uint32_t data);
// 제거됨: void processBulkDataFromCPU1(void);

#endif // CSU_IPC_CM_H
