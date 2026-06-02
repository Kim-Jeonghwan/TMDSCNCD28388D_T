/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_Ethernet.h
    Description      : UDP 프로토콜 처리 계층 (규격서 Payload/ACK MSG Format 준수)
    Last Updated     : 2026. 06. 02. (PC 수신 포트를 50002로 변경하여 포트 충돌 방지)
**********************************************************************/

#ifndef CSU_ETHERNET_H
#define CSU_ETHERNET_H

#include "main.h"

/* ---------------------------------------------------------------
 * 장치 ID / 메시지 코드 상수 (규격서 정의)
 * --------------------------------------------------------------- */
#define ETH_SRC_ID_DSP        (0x10U)   /* DSP 장치 ID */
#define ETH_DST_ID_PC         (0x20U)   /* PC  장치 ID */
#define ETH_MSG_CODE_MONITOR  (0x10U)   /* 온도+시퀀스 모니터링 코드 */
#define ETH_MSG_CODE_ACK      (0xFFU)   /* ACK 메시지 코드 */

#define ETH_REQ_ACK_NONE      (0xFFU)   /* ACK 미요청 (Reflect 타입) */
#define ETH_REQ_ACK_REQ       (0x01U)   /* ACK 요청   (Update  타입) */

#define ETH_ACK_OK            (0x10U)   /* ACK 응답 */
#define ETH_ACK_NACK          (0x11U)   /* NACK 응답 */
#define ETH_ACKINFO_OK        (0x0000U) /* 정상 */
#define ETH_ACKINFO_CHECKSUM  (0x0001U) /* Checksum 오류 */

#define ETH_PRIORITY_NORMAL   (0x02U)   /* 우선순위: 일반 */
#define ETH_SEND_COUNT_INIT   (0x01U)   /* Send Count 초기값 */

/* ---------------------------------------------------------------
 * 네트워크 설정 (고정 IP, ARP 없음)
 * --------------------------------------------------------------- */
/* DSP IP: 192.168.100.10 */
#define ETH_DSP_IP0  (192U)
#define ETH_DSP_IP1  (168U)
#define ETH_DSP_IP2  (100U)
#define ETH_DSP_IP3  (10U)

/* PC IP: 192.168.100.100 */
#define ETH_PC_IP0   (192U)
#define ETH_PC_IP1   (168U)
#define ETH_PC_IP2   (100U)
#define ETH_PC_IP3   (100U)

/* DSP MAC: A8:63:F2:00:38:88 */
#define ETH_DSP_MAC0 (0xA8U)
#define ETH_DSP_MAC1 (0x63U)
#define ETH_DSP_MAC2 (0xF2U)
#define ETH_DSP_MAC3 (0x00U)
#define ETH_DSP_MAC4 (0x38U)
#define ETH_DSP_MAC5 (0x88U)

/* PC MAC: EC:9A:0C:14:E8:4B (실제 NX USB2.0 이더넷 어댑터 물리 주소 반영) */
#define ETH_PC_MAC0  (0xECU)
#define ETH_PC_MAC1  (0x9AU)
#define ETH_PC_MAC2  (0x0CU)
#define ETH_PC_MAC3  (0x14U)
#define ETH_PC_MAC4  (0xE8U)
#define ETH_PC_MAC5  (0x4BU)

/* UDP 포트 */
#define ETH_DSP_RX_PORT  (5001U)   /* DSP 수신 포트 */
#define ETH_PC_RX_PORT   (50002U)  /* PC  수신 포트 (5000번 포트 충돌 회피용) */

/* MSG Header 크기 정의 */
#define ETH_MSG_HEADER_SIZE     (12U)
#define ETH_PAYLOAD_DATA_SIZE   (4U)    /* 온도+시퀀스 Reflect 데이터 크기 */
#define ETH_PC_DATA_SIZE        (2U)    /* PC→DSP Update 데이터 크기 */
#define ETH_ACK_DATA_SIZE       (4U)    /* ACK 데이터 크기 (Code Info + Ack Info) */
#define ETH_CHECKSUM_SIZE       (2U)

/* ---------------------------------------------------------------
 * CM↔CPU1 공유 데이터 구조체
 * --------------------------------------------------------------- */
typedef struct
{
    uint16_t DspTemp;   /* 온도 x10 스케일, Little Endian */
    uint8_t  SeqNum;    /* 시퀀스 번호 */
    uint8_t  Status;    /* 상태 바이트 */
} stEthSharedData;

/* 공유 데이터 전역 변수 (CSU_IPC.c 에서 갱신, CSU_Ethernet.c 에서 참조) */
extern stEthSharedData g_xEthTxData;   /* CPU1 → CM → 이더넷 Tx */
extern stEthSharedData g_xEthRxData;   /* 이더넷 Rx → CM → CPU1 */

/* ---------------------------------------------------------------
 * 함수 프로토타입
 * --------------------------------------------------------------- */
void buildAndSendUdpPacket(void);
void processReceivedEthernetPacket(uint8_t *pPacket, uint16_t length);
void sendAckResponse(uint8_t ackResult, uint16_t ackInfo,
                     uint32_t timestamp, uint8_t targetCode);

#endif /* CSU_ETHERNET_H */
