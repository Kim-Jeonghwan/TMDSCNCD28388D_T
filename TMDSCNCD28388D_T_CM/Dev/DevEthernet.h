/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevEthernet.h
    Description      : Ethernet EMAC 드라이버 계층 헤더 (MII 모드, DP83822 PHY)
    Last Updated     : 2026. 06. 05. (코드 주석 포맷팅 및 한글화)
**********************************************************************/

#ifndef DEV_ETHERNET_H
#define DEV_ETHERNET_H

#include "main.h"

/* ---------------------------------------------------------------
 * EMAC 기본 주소 (hw_memmap.h 정의값 사용)
 * EMAC_BASE     = 0x400C0000U
 * EMAC_SS_BASE  = 0x400C2000U
 * --------------------------------------------------------------- */

/* Rx 버퍼 크기 및 채널 개수 */
#define ETH_RX_NUM_PKT_DESC    (3U)     /* Rx 디스크립터(패킷 버퍼) 개수 */
#define ETH_RX_BUF_SIZE        (1536U)  /* 단일 Rx 버퍼 크기 (1518B 이더넷 최대 + 여유) */
#define ETH_TX_BUF_SIZE        (256U)   /* Tx 버퍼 크기 (최대 UDP 패킷 61B 대비 여유) */

/* EMAC 핸들 전역 변수 (CSU_Ethernet.c 에서 사용) */
extern Ethernet_Handle g_hEMAC;

/* Tx 버퍼 (CSU_Ethernet.c 에서 패킷 조립 후 공유 사용) */
extern uint8_t g_ucTxBuf[ETH_TX_BUF_SIZE];

/* ---------------------------------------------------------------
 * 함수 프로토타입
 * --------------------------------------------------------------- */

/* EMAC 초기화 (GPIO 제외 - CPU1 DevDspInit.c 에서 GPIO 설정) */
void Initial_Ethernet(void);

/* 이더넷 수신 폴링 태스크 (main loop 에서 주기 호출) */
void updateEthernetTask(void);

/* Ethernet_InitConfig 에 등록되는 콜백 함수들 */
Ethernet_Pkt_Desc *App_ethGetPacketBuffer(void);
Ethernet_Pkt_Desc *App_ethRxCallback(Ethernet_Handle hApp, Ethernet_Pkt_Desc *pPkt);
void               App_ethTxCallback(Ethernet_Handle hApp, Ethernet_Pkt_Desc *pPkt);

/* LLD 드라이버 내 하드폴트 방지용 인터럽트 제어 콜백 래퍼 */
void Platform_enableCoreInterrupt(void);
void Platform_disableCoreInterrupt(void);

#endif /* DEV_ETHERNET_H */

