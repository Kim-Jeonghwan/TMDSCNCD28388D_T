/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_IPC.h
    Description      : IPC Protocol (CM to CPU1) 및 이더넷 공유 메모리(GS0/GS1) 설정
    Last Updated     : 2026. 06. 04. (CM 기동 완료 READY 명령어 추가)
**********************************************************************/

#ifndef CSU_IPC_H
#define CSU_IPC_H

#include "main.h"

// IPC 통신용 공용 데이터 구조체 (32비트 정렬)
typedef struct {
    uint32_t Command;       // 명령어
    uint32_t Status;        // 상태 플래그
    uint32_t Address;       // 메모리 주소
    uint32_t Payload[16];   // 실제 데이터 배열
} stIpcDataPacket;

// Message RAM 정의 (CM View)
#define IPC_CPU1_TO_CM_MSGRAM_ADDR    0x20080000U
#define IPC_CM_TO_CPU1_MSGRAM_ADDR    0x20082000U

// --- 이더넷 패킷 전용 공유 메모리 설정 ---
// 1. 이더넷 패킷 데이터 구조 (GSRAM에 배치될 데이터)
typedef struct {
    uint32_t Length;        // 패킷 길이 (Bytes)
    uint32_t Reserved;      // 64비트 정렬용
    uint8_t  Data[1514];    // 실제 이더넷 프레임 데이터 (MTU 기준)
} stEthPacketBuffer;

// 2. GSRAM 주소 정의 (CM View)
#define GS0_CPU_START_ADDR    0x0000D000U  // C28x GS0 Start
#define GS0_CM_START_ADDR     0x20014000U  // CM GS0 Start
#define GS1_CPU_START_ADDR    0x0000E000U  // C28x GS1 Start
#define GS1_CM_START_ADDR     0x20016000U  // CM GS1 Start

// 3. 주소 변환 매크로 (CM Byte Addr <-> CPU Word Addr)
#define CONVERT_CM_TO_CPU_ADDR(cm_addr)  (GS0_CPU_START_ADDR + (((cm_addr) - GS0_CM_START_ADDR) >> 1))
#define CONVERT_CPU_TO_CM_ADDR(cpu_addr) (GS0_CM_START_ADDR + (((cpu_addr) - GS0_CPU_START_ADDR) << 1))

// IPC 명령어 정의
#define IPC_CMD_ETH_RCV_NOTIFY    0x1001U  /* CM -> CPU1: 패킷 수신 알림 */
#define IPC_CMD_ETH_XMT_REQUEST   0x1002U  /* CPU1 -> CM: 패킷 송신 요청 */
#define IPC_CMD_CPU1_ETH_TX_DATA  0x2001U  /* CPU1 -> CM: 온도+시퀀스 전달 */
#define IPC_CMD_CM_ETH_RX_DATA    0x2002U  /* CM -> CPU1: 수신 SeqNum/Cmd 전달 */
#define IPC_CMD_CM_BOOT_READY     0x3001U  /* CM -> CPU1: CM 기동 및 주변기기 초기화 완료 */

extern volatile stIpcDataPacket *pxIpcCpu1ToCm;
extern volatile stIpcDataPacket *pxIpcCmToCpu1;

void recvIpcCpu1Message(uint32_t command, uint32_t addr, uint32_t data);
void processBulkDataFromCPU1(void);

#endif // CSU_IPC_H
