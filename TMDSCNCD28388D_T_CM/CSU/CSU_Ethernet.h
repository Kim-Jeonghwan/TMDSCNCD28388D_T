/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Ethernet.h
    Version          : 00.04
    Description      : UDP 프로토콜 처리 계층
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비 및 매크로 이동)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (작성자 기입, 매크로 상수 이동 및 이력 보완)
 * 2026. 06. 22. - 헤더 가드 매크로를 규칙에 맞춰 대문자(CSU_ETHERNET_H)로 규격화
 * 2026. 06. 22. - 물리 파일명을 csu_Ethernet.h로 소문자 정정
 * 2026. 06. 19. - 변수명 규칙 적용 (xCsuEth -> xEthApp 변경)
 * 2026. 06. 19. - 이더넷 전역 변수 캡슐화 적용
 * 2026. 06. 19. - Phase 5: 사인파 추가 및 PC 요청 응답 구조 변경
 */

#ifndef CSU_ETHERNET_H
#define CSU_ETHERNET_H

#include "main_cm.h"

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
/* DSP IP: 192.168.200.10 */
#define ETH_DSP_IP0  (192U)
#define ETH_DSP_IP1  (168U)
#define ETH_DSP_IP2  (200U)
#define ETH_DSP_IP3  (10U)

/* PC IP: 192.168.200.100 */
#define ETH_PC_IP0   (192U)
#define ETH_PC_IP1   (168U)
#define ETH_PC_IP2   (200U)
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
#define ETH_PC_RX_PORT   (5000U)   /* PC  수신 포트 (기본 포트) */

/* MSG Header 크기 정의 */
#define ETH_MSG_HEADER_SIZE     (12U)
#define ETH_PAYLOAD_DATA_SIZE   (8U)    /* 온도+시퀀스+상태+사인파 Reflect 데이터 크기 */
#define ETH_PC_DATA_SIZE        (2U)    /* PC→DSP Update 데이터 크기 */
#define ETH_ACK_DATA_SIZE       (4U)    /* ACK 데이터 크기 (Code Info + Ack Info) */
#define ETH_CHECKSUM_SIZE       (2U)

/* 이더넷 TX/RX 드라이버 레벨 상수 및 핀 제어 매크로 */
#define ETH_TX_NUM_PKT_DESC  (4U)

#define ETH_HDR_DST_OFFSET   (0U)    /* 목적지 MAC (6B) */
#define ETH_HDR_SRC_OFFSET   (6U)    /* 출발지 MAC (6B) */
#define ETH_HDR_TYPE_OFFSET  (12U)   /* EtherType (2B): 0x0800=IPv4 */
#define ETH_HDR_SIZE         (14U)

#define IP_HDR_OFFSET        ETH_HDR_SIZE
#define IP_HDR_VER_IHL       (0x45U) /* IPv4, 20B 헤더 */
#define IP_HDR_DSCP          (0x00U)
#define IP_TTL               (64U)
#define IP_PROTO_UDP         (0x11U)
#define IP_HDR_SIZE          (20U)

#define UDP_HDR_OFFSET       (ETH_HDR_SIZE + IP_HDR_SIZE)
#define UDP_HDR_SIZE         (8U)

#define PAYLOAD_OFFSET       (ETH_HDR_SIZE + IP_HDR_SIZE + UDP_HDR_SIZE)

#define TX_REFLECT_FRAME_SIZE  (64U) /* ETH(14) + IP(20) + UDP(8) + Payload(22) */
#define TX_ACK_FRAME_SIZE      (60U) /* ETH + IP + UDP + 18B payload */
#define MIN_RX_FRAME_SIZE      (ETH_HDR_SIZE + IP_HDR_SIZE + UDP_HDR_SIZE + ETH_MSG_HEADER_SIZE + ETH_CHECKSUM_SIZE)

#define ETH_LED_PIN 146U
#define ETH_LED_ON()  GPIO_writePin(ETH_LED_PIN, 1U)

/* ---------------------------------------------------------------
 * CM↔CPU1 공유 데이터 구조체
 * --------------------------------------------------------------- */
typedef struct
{
    float32_t WaveVal;  /* 파형 값 */
    uint16_t DspTemp;   /* 온도 x10 스케일, Little Endian */
    uint8_t  SeqNum;    /* 시퀀스 번호 */
    uint8_t  WaveType;  /* 선택된 파형 종류 */
} stEthSharedData;

/* ---------------------------------------------------------------
 * CSU 계층 이더넷 통신 상태 구조체 캡슐화
 * --------------------------------------------------------------- */
typedef struct {
    stEthSharedData txData;     /* CPU1 → CM → 이더넷 Tx */
    stEthSharedData rxData;     /* 이더넷 Rx → CM → CPU1 */
    uint8_t realPcMac[6];       /* 동적 학습된 PC의 물리적 MAC 주소 */
    uint16_t lastRxSrcPort;     /* 마지막 수신 패킷의 출발지 포트 번호 */
} stEthAppState;

/* 구조체 인스턴스 (csu_Ipc_cm.c 등에서 공유 사용) */
extern stEthAppState xEthApp;

/* 이더넷 활동(Tx/Rx) LED 표시용 타이머 (1ms 주기 감소) */
extern uint16_t ethActivityTimer;

/* ---------------------------------------------------------------
 * 함수 프로토타입
 * --------------------------------------------------------------- */
void buildAndSendUdpPacket(uint32_t rxTimestamp);
void processReceivedEthernetPacket(uint8_t *pPacket, uint16_t length);
void sendAckResponse(uint8_t ackResult, uint16_t ackInfo,
                     uint32_t timestamp, uint8_t targetCode);

#endif /* CSU_ETHERNET_H */
