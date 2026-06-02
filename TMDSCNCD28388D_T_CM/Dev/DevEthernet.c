/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevEthernet.c
    Description      : EMAC 드라이버 초기화 및 Rx/Tx 콜백 구현 (MII, DP83822)
    Last Updated     : 2026. 06. 02. (정적 구조체 static xInitCfg 설계를 적용하여 메모리 오염에 따른 CM 하드 폴트 원천 제거)
**********************************************************************/

/*
 * [회로도 GPIO 핀 매핑] (CPU1 DevDspInit.c 에서 GPIO 핀 MUX 설정)
 *   GPIO44  → ENET_MII_TX_CLK
 *   GPIO118 → ENET_MII_TX_EN
 *   GPIO75  → ENET_MII_TX_DATA0
 *   GPIO122 → ENET_MII_TX_DATA1
 *   GPIO123 → ENET_MII_TX_DATA2
 *   GPIO124 → ENET_MII_TX_DATA3
 *   GPIO111 → ENET_MII_RX_CLK
 *   GPIO112 → ENET_MII_RX_DV
 *   GPIO113 → ENET_MII_RX_ERR
 *   GPIO114 → ENET_MII_RX_DATA0
 *   GPIO115 → ENET_MII_RX_DATA1
 *   GPIO116 → ENET_MII_RX_DATA2
 *   GPIO117 → ENET_MII_RX_DATA3
 *   GPIO105 → ENET_MDIO_CLK
 *   GPIO106 → ENET_MDIO_DATA
 *
 * [CM 클럭] DevDspInit.c 에서 SYSCTL_CMCLKOUT_DIV_2 → 100 MHz
 *
 * [EMAC 기본 주소]
 *   EMAC_BASE    = 0x400C0000U  (Ethernet MAC 레지스터)
 *   EMAC_SS_BASE = 0x400C2000U  (Ethernet SS Wrapper 레지스터)
 */

#include "DevEthernet.h"
#include "CSU_Ethernet.h"

/* ---------------------------------------------------------------
 * 전역 변수
 * --------------------------------------------------------------- */

/* EMAC 핸들 */
Ethernet_Handle g_hEMAC = (Ethernet_Handle)0U;

/* 디버깅 용: 이더넷 초기화 함수 리턴 결과값 저장 */
volatile uint32_t g_uiEthInitRet = 0U;

/* Tx 버퍼 (CSU_Ethernet.c 에서 패킷 조립 후 이 버퍼 사용) */
uint8_t g_ucTxBuf[ETH_TX_BUF_SIZE];

/* Rx 버퍼 및 디스크립터 풀 */
static uint8_t           s_ucRxBuf[ETH_RX_NUM_PKT_DESC][ETH_RX_BUF_SIZE];
static Ethernet_Pkt_Desc s_xRxPktDesc[ETH_RX_NUM_PKT_DESC];
static Ethernet_Pkt_Desc s_xTxPktDesc;

/* Rx 버퍼 순환 인덱스 */
static uint8_t s_ucRxBufIdx = 0U;

/* ---------------------------------------------------------------
 * static 함수 선언
 * --------------------------------------------------------------- */
static void initRxDescriptors(void);

/* ---------------------------------------------------------------
 * Rx 디스크립터 풀 초기화
 * --------------------------------------------------------------- */
/*
@funtion    static void initRxDescriptors(void)
@brief      Rx 패킷 디스크립터 배열을 초기화하고 수신 버퍼를 할당합니다.
@param      void
@return     static void
*/
static void initRxDescriptors(void)
{
    uint8_t i = 0U;

    for (i = 0U; i < ETH_RX_NUM_PKT_DESC; i++)
    {
        s_xRxPktDesc[i].dataBuffer  = s_ucRxBuf[i];
        s_xRxPktDesc[i].bufferLength = ETH_RX_BUF_SIZE;
        s_xRxPktDesc[i].dataOffset  = 0U;
        s_xRxPktDesc[i].validLength = 0U;
        s_xRxPktDesc[i].flags       = 0U;
        s_xRxPktDesc[i].nextPacketDesc = NULL;
    }
}

/* ---------------------------------------------------------------
 * EMAC 초기화
 * --------------------------------------------------------------- */
/*
@funtion    void Initial_Ethernet(void)
@brief      EMAC 드라이버 초기화 (MII 모드, DP83822 PHY, 외부 클럭)
@param      void
@return     void
*/
void Initial_Ethernet(void)
{
    Ethernet_InitInterfaceConfig xIfCfg;
    static Ethernet_InitConfig   xInitCfg; /* static 선언으로 스택 오버플로우 원천 예방 및 드라이버 오염 방지 */
    uint32_t                     uiRet = 0U;

    /* --- Rx 디스크립터 풀 초기화 --- */
    initRxDescriptors();
    (void)memset(g_ucTxBuf, 0, ETH_TX_BUF_SIZE);
    (void)memset(&s_xTxPktDesc, 0, sizeof(s_xTxPktDesc));

    /* -------------------------------------------------------
     * Step 1: 인터페이스 설정 구조체 구성
     * ------------------------------------------------------- */
    (void)memset(&xIfCfg, 0, sizeof(xIfCfg));
    xIfCfg.ssbase           = EMAC_SS_BASE;
    xIfCfg.enet_base        = EMAC_BASE;
    xIfCfg.phyMode          = ETHERNET_SS_PHY_INTF_SEL_MII;
    xIfCfg.clockSel         = ETHERNET_SS_CLK_SRC_EXTERNAL; /* PHY 25MHz 외부 공급 */
    xIfCfg.localPhyAddress  = 0U;
    xIfCfg.remotePhyAddress = 0U;

    /* Platform 콜백 함수 (platform_port.c 에 구현) */
    xIfCfg.ptrPlatformPeripheralEnable   = &Platform_enablePeripheral;
    xIfCfg.ptrPlatformPeripheralReset    = &Platform_resetPeripheral;
    xIfCfg.ptrPlatformInterruptEnable    = &Platform_enableInterrupt;
    xIfCfg.ptrPlatformInterruptDisable   = &Platform_disableInterrupt;
    xIfCfg.ptrCoreInterruptEnable        = NULL; /* CM ARM: NVIC 직접 제어 불필요 */
    xIfCfg.ptrCoreInterruptDisable       = NULL;

    /* EMAC 관련 인터럽트 번호 등록 (CM 인터럽트 벡터 테이블 기준) */
    xIfCfg.interruptNum[0U] = INT_EMAC;     /* EMAC 일반 인터럽트 */
    xIfCfg.interruptNum[1U] = INT_EMAC_TX0; /* TX Channel 0 완료 인터럽트 */
    xIfCfg.interruptNum[2U] = INT_EMAC_RX0; /* RX Channel 0 완료 인터럽트 */

    /* SysCtl 주변장치 번호 (EMAC 클럭/리셋 용) */
    xIfCfg.peripheralNum = SYSCTL_PERIPH_CLK_ENET;

    /* -------------------------------------------------------
     * Step 2: 인터페이스 초기화 (SS 레지스터 설정)
     * ------------------------------------------------------- */
    Ethernet_initInterface(xIfCfg);

    /* -------------------------------------------------------
     * Step 3: EMAC 초기화 설정 구조체 구성
     * ------------------------------------------------------- */
    (void)memset(&xInitCfg, 0, sizeof(xInitCfg));
    Ethernet_getInitConfig(&xInitCfg);  /* 기본값 로드 */

    /* MAC 주소 설정: A8:63:F2:00:38:88 (Little Endian) */
    xInitCfg.macAddr = NULL; /* Ethernet_setMACAddr 로 별도 설정 예정 */

    /* Rx 버퍼 공급 및 콜백 등록 */
    xInitCfg.pfcbGetPacket     = &App_ethGetPacketBuffer;
    xInitCfg.pfcbRxPacket      = &App_ethRxCallback;
    xInitCfg.pfcbFreePacket    = &App_ethTxCallback;
    xInitCfg.rxBuffer          = s_ucRxBuf[0U];

    /* DMA 채널 수: 1채널 */
    xInitCfg.numChannels = 1U;
    xInitCfg.pktMTU      = (uint32_t)ETH_RX_BUF_SIZE;

    /* -------------------------------------------------------
     * Step 4: EMAC 핸들 획득 및 초기화
     * ------------------------------------------------------- */
    uiRet = Ethernet_getHandle((Ethernet_Handle)1U, &xInitCfg, &g_hEMAC);

    g_uiEthInitRet = uiRet;

    if (uiRet != ETHERNET_RET_SUCCESS)
    {
        /* 초기화 실패 처리: 핸들 무효화 */
        g_hEMAC = (Ethernet_Handle)0U;
    }
    else
    {
        /* -------------------------------------------------------
         * Step 5: MAC 주소 설정 및 TX/RX 활성화
         * ------------------------------------------------------- */
        /* MAC 주소 포맷 교정 (High 2B, Low 4B Little-Endian 포맷팅) */
        uint32_t macHigh = ((uint32_t)ETH_DSP_MAC5 << 8U) | (uint32_t)ETH_DSP_MAC4;
        uint32_t macLow  = ((uint32_t)ETH_DSP_MAC3 << 24U) | ((uint32_t)ETH_DSP_MAC2 << 16U) | ((uint32_t)ETH_DSP_MAC1 << 8U) | (uint32_t)ETH_DSP_MAC0;
        Ethernet_setMACAddr(EMAC_BASE, 0U, macHigh, macLow, ETHERNET_CHANNEL_0);

        /* Duplex, Loopback 설정 및 이더넷 송수신 모듈 하드웨어 기동(TE 및 RE) 플래그 조립 */
        uint32_t macFlags = ((uint32_t)ETHERNET_MAC_CONFIGURATION_DM_FULL_DUPLEX << ETHERNET_MAC_CONFIGURATION_DM_S) |
                            ((uint32_t)ETHERNET_MAC_CONFIGURATION_LM_LOOPBACK_DISABLED << ETHERNET_MAC_CONFIGURATION_LM_S) |
                            ETHERNET_MAC_CONFIGURATION_TE | ETHERNET_MAC_CONFIGURATION_RE;
        
        /* MAC 설정 반영 및 통신 기동 */
        Ethernet_setMACConfiguration(EMAC_BASE, macFlags);
    }
}

/* ---------------------------------------------------------------
 * 이더넷 수신 폴링 태스크 (main loop 에서 호출)
 * --------------------------------------------------------------- */
/*
@funtion    void updateEthernetTask(void)
@brief      이더넷 수신 완료 큐를 폴링하여 수신된 패킷을 처리합니다.
@param      void
@return     void
@remark
    - EMAC 핸들이 유효한 경우에만 동작합니다.
    - 수신된 패킷은 App_ethRxCallback 콜백을 통해 처리됩니다.
*/
void updateEthernetTask(void)
{
    if (g_hEMAC != (Ethernet_Handle)0U)
    {
        Ethernet_removePacketsFromRxQueue(&((Ethernet_Device *)g_hEMAC)->dmaObj.rxDma[0U], ETHERNET_COMPLETION_NORMAL);
    }
}

/* ---------------------------------------------------------------
 * 콜백: EMAC Rx 버퍼 공급 (EMAC 드라이버가 호출)
 * --------------------------------------------------------------- */
/*
@funtion    Ethernet_Pkt_Desc *App_ethGetPacketBuffer(void)
@brief      EMAC 드라이버가 Rx 수신을 위해 빈 버퍼를 요청할 때 호출됩니다.
@param      void
@return     Ethernet_Pkt_Desc *: 빈 Rx 디스크립터 포인터 (NULL이면 버퍼 고갈)
@remark
    - 순환 인덱스(s_ucRxBufIdx)로 풀에서 버퍼를 순환 공급합니다.
    - CWE-476: 반환 전 NULL 체크 없음(항상 유효 배열 반환)
*/
Ethernet_Pkt_Desc *App_ethGetPacketBuffer(void)
{
    Ethernet_Pkt_Desc *pDesc = NULL;

    pDesc = &s_xRxPktDesc[s_ucRxBufIdx];
    pDesc->dataBuffer   = s_ucRxBuf[s_ucRxBufIdx];
    pDesc->bufferLength = ETH_RX_BUF_SIZE;
    pDesc->dataOffset   = 0U;
    pDesc->validLength  = 0U;
    pDesc->flags        = 0U;
    pDesc->nextPacketDesc = NULL;

    /* 순환 인덱스 증가 */
    s_ucRxBufIdx = (s_ucRxBufIdx + 1U) % ETH_RX_NUM_PKT_DESC;

    return pDesc;
}

/* ---------------------------------------------------------------
 * 콜백: 수신 패킷 처리 (EMAC 드라이버가 패킷 수신 시 호출)
 * --------------------------------------------------------------- */
/*
@funtion    Ethernet_Pkt_Desc *App_ethRxCallback(Ethernet_Handle hApp, Ethernet_Pkt_Desc *pPkt)
@brief      EMAC 드라이버가 패킷 수신 완료 시 호출하는 콜백 함수입니다.
@param      hApp: 애플리케이션 핸들 (미사용)
@param      pPkt: 수신된 패킷 디스크립터 포인터
@return     Ethernet_Pkt_Desc *: 다음 Rx를 위해 새 버퍼 제공 (App_ethGetPacketBuffer 반환값)
@remark
    - CWE-476: pPkt NULL 체크 수행
*/
Ethernet_Pkt_Desc *App_ethRxCallback(Ethernet_Handle hApp, Ethernet_Pkt_Desc *pPkt)
{
    (void)hApp; /* 미사용 파라미터 */

    if (pPkt != NULL)
    {
        if (pPkt->dataBuffer != NULL)
        {
            /* dataLength -> validLength 로 전달하여 프로세싱 */
            processReceivedEthernetPacket(pPkt->dataBuffer, (uint16_t)pPkt->validLength);
        }
    }

    /* 새 빈 버퍼 제공 */
    return App_ethGetPacketBuffer();
}

/* ---------------------------------------------------------------
 * 콜백: Tx 완료 후 버퍼 반환 (EMAC 드라이버가 Tx 완료 시 호출)
 * --------------------------------------------------------------- */
/*
@funtion    void App_ethTxCallback(Ethernet_Handle hApp, Ethernet_Pkt_Desc *pPkt)
@brief      EMAC 드라이버의 Tx 완료 콜백. Tx 버퍼를 반환 처리합니다.
@param      hApp: 애플리케이션 핸들 (미사용)
@param      pPkt: 전송 완료된 패킷 디스크립터 포인터 (미사용)
@return     void
*/
void App_ethTxCallback(Ethernet_Handle hApp, Ethernet_Pkt_Desc *pPkt)
{
    (void)hApp; /* 미사용 파라미터 */
    (void)pPkt; /* 미사용 파라미터 */
    /* g_ucTxBuf 는 정적 배열이므로 별도 해제 불필요 */
}
