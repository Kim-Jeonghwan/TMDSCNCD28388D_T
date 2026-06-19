/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Ethernet.c
    Version          : 00.02
    Description      : EMAC 드라이버 초기화 및 Rx 하드웨어 인터럽트 구현
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (이더넷 전역 변수 캡슐화)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - 변수명 규칙 적용 (xHalEth -> xEthDriver 변경)
 * 2026. 06. 19. - 이더넷 전역 변수 캡슐화 적용
 * 2026. 06. 19. - (Phase 4: 이더넷 RX 폴링 -> 인터럽트 전환)
 * 2026. 06. 05. - 코드 주석 포맷팅 및 한글화
 */

/*
 * [회로도 GPIO 핀 매핑] (CPU1 hal_DspInit.c 에서 GPIO 핀 MUX 설정)
 * GPIO44  → ENET_MII_TX_CLK
 * GPIO118 → ENET_MII_TX_EN
 * GPIO75  → ENET_MII_TX_DATA0
 * GPIO122 → ENET_MII_TX_DATA1
 * GPIO123 → ENET_MII_TX_DATA2
 * GPIO124 → ENET_MII_TX_DATA3
 * GPIO111 → ENET_MII_RX_CLK
 * GPIO112 → ENET_MII_RX_DV
 * GPIO113 → ENET_MII_RX_ERR
 * GPIO114 → ENET_MII_RX_DATA0
 * GPIO115 → ENET_MII_RX_DATA1
 * GPIO116 → ENET_MII_RX_DATA2
 * GPIO117 → ENET_MII_RX_DATA3
 * GPIO105 → ENET_MDIO_CLK
 * GPIO106 → ENET_MDIO_DATA
 *
 * [CM 클럭] hal_DspInit.c 에서 AUXPLL 기반 125 MHz 설정
 *
 * [EMAC 기본 주소]
 * EMAC_BASE    = 0x400C0000U  (Ethernet MAC 레지스터)
 * EMAC_SS_BASE = 0x400C2000U  (Ethernet SS Wrapper 레지스터)
 */

#include "hal_Ethernet.h"

/* ---------------------------------------------------------------
 * 전역 변수
 * --------------------------------------------------------------- */

/* HAL 계층 이더넷 상태 인스턴스 초기화 */
stEthDriverState xEthDriver = {
    .hEMAC = (Ethernet_Handle)0U,
    .txBuf = {0U},
    .initRet = 0U
};

/* Rx 버퍼 및 디스크립터 풀 */
static uint8_t           s_ucRxBuf[ETH_RX_NUM_PKT_DESC][ETH_RX_BUF_SIZE];
static Ethernet_Pkt_Desc s_xRxPktDesc[ETH_RX_NUM_PKT_DESC];

/* Rx 버퍼 순환 인덱스 */
static uint8_t s_ucRxBufIdx = 0U;

/* ---------------------------------------------------------------
 * static 함수 선언
 * --------------------------------------------------------------- */
static void initRxDescriptors(void);
static void isr_EmacRx0(void);

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
 * LLD 드라이버 내 하드폴트 방지용 인터럽트 제어 콜백 래퍼 구현
 * --------------------------------------------------------------- */
/*
@funtion    void Platform_enableCoreInterrupt(void)
@brief      CM 코어의 전역 인터럽트를 활성화합니다.
@param      void
@return     void
*/
void Platform_enableCoreInterrupt(void)
{
    (void)Interrupt_enableInProcessor();
}

/*
@funtion    void Platform_disableCoreInterrupt(void)
@brief      CM 코어의 전역 인터럽트를 비활성화합니다.
@param      void
@return     void
*/
void Platform_disableCoreInterrupt(void)
{
    (void)Interrupt_disableInProcessor();
}

/* ---------------------------------------------------------------
 * EMAC 초기화
 * --------------------------------------------------------------- */
/*
@function    Initial_Ethernet
@brief      EMAC 드라이버 초기화 (MII 모드, DP83822 PHY, 외부 클럭)
@param      void
@return     void
*/
void Initial_Ethernet(void)
{
    Ethernet_InitInterfaceConfig xIfCfg;
    uint32_t                     uiRet = 0U;

    /* --- Rx 디스크립터 풀 초기화 --- */
    initRxDescriptors();
    (void)memset(xEthDriver.txBuf, 0U, ETH_TX_BUF_SIZE);

    /* -------------------------------------------------------
     * Step 1: 인터페이스 설정 구조체 구성
     * ------------------------------------------------------- */
    (void)memset(&xIfCfg, 0U, sizeof(xIfCfg));
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
    xIfCfg.ptrCoreInterruptEnable        = &Platform_enableCoreInterrupt;  /* LLD 하드폴트 방지용 인터럽트 ON 콜백 */
    xIfCfg.ptrCoreInterruptDisable       = &Platform_disableCoreInterrupt; /* LLD 하드폴트 방지용 인터럽트 OFF 콜백 */

    /* EMAC 관련 인터럽트 번호 등록 (CM 인터럽트 벡터 테이블 기준) */
    /* hw_ints.h 규격에 맞춰 표준 매크로 명칭 유지 */
    xIfCfg.interruptNum[0U] = INT_EMAC;     /* EMAC 일반 인터럽트 */
    xIfCfg.interruptNum[1U] = INT_EMAC_TX0; /* TX Channel 0 완료 인터럽트 */
    xIfCfg.interruptNum[2U] = INT_EMAC_RX0; /* RX Channel 0 완료 인터럽트 */

    /* SysCtl 주변장치 번호 (EMAC 클럭/리셋 용) */
    xIfCfg.peripheralNum = SYSCTL_PERIPH_CLK_ENET;

    /* -------------------------------------------------------
     * Step 2: 인터페이스 초기화 (SS 레지스터 설정) 및 전역 InitConfig 포인터 획득
     * ------------------------------------------------------- */
    Ethernet_InitConfig *pInitCfg = Ethernet_initInterface(xIfCfg);
    
    if (pInitCfg != NULL)
    {
        /* -------------------------------------------------------
         * Step 3: EMAC 초기화 설정 구조체 구성
         * ------------------------------------------------------- */
        Ethernet_getInitConfig(pInitCfg);  /* 기본값 로드 */

    /* MAC 주소 설정: A8:63:F2:00:38:88 (Little Endian) */
    pInitCfg->macAddr = NULL; /* Ethernet_setMACAddr 로 별도 설정 예정 */

    /* Rx 버퍼 공급 및 콜백 등록 */
    pInitCfg->pfcbGetPacket     = &App_ethGetPacketBuffer;
    pInitCfg->pfcbRxPacket      = &App_ethRxCallback;
    pInitCfg->pfcbFreePacket    = &App_ethTxCallback;
    pInitCfg->rxBuffer          = s_ucRxBuf[0U];

    /* DMA 채널 수: 1채널 */
    pInitCfg->numChannels = 1U;
    pInitCfg->pktMTU      = (uint32_t)ETH_RX_BUF_SIZE;

    /* -------------------------------------------------------
     * Step 4: EMAC 핸들 획득 및 초기화
     * ------------------------------------------------------- */
        uiRet = Ethernet_getHandle((Ethernet_Handle)1U, pInitCfg, &xEthDriver.hEMAC);
    }
    else
    {
        uiRet = ETHERNET_ERR_INVALID_PARAM;
    }

    xEthDriver.initRet = uiRet;

    if (uiRet != ETHERNET_RET_SUCCESS)
    {
        /* 초기화 실패 처리: 핸들 무효화 */
        xEthDriver.hEMAC = (Ethernet_Handle)0U;
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

        /* --- [개정 완료] DP83822 이더넷 PHY LED_1 (초록색) 강제 활성화 --- */
        
        // 1. IOCTRL1 (0x0462) 레지스터 설정: LED_1 기능을 수행하도록 MUX 설정 복원/확인
        Ethernet_writePHYRegister(EMAC_BASE, 0x0DU, 0x001FU);  /* Address mode, DEVAD = 0x1F */
        Ethernet_writePHYRegister(EMAC_BASE, 0x0EU, 0x0462U);  /* ADDAR = 0x0462 (IOCTRL1) */
        Ethernet_writePHYRegister(EMAC_BASE, 0x0DU, 0x401FU);  /* Data mode, no post increment */
        uint16_t uiIoCtrl1 = Ethernet_readPHYRegister(EMAC_BASE, 0x0EU);
        
        uiIoCtrl1 &= ~(0x0007U); /* Bits [2:0] GPIO1/LED_1 MUX clear */
        uiIoCtrl1 |= 0x0000U;    /* 000b = Normal LED_1 operation Mode 지정 */
        Ethernet_writePHYRegister(EMAC_BASE, 0x0EU, uiIoCtrl1);
        
        // 2. LEDCFG1 (0x0460) 레지스터 설정: LED_1 구동 기본 모드 클리어
        Ethernet_writePHYRegister(EMAC_BASE, 0x0DU, 0x001FU);  /* Address mode */
        Ethernet_writePHYRegister(EMAC_BASE, 0x0EU, 0x0460U);  /* ADDAR = 0x0460 (LEDCFG1) */
        Ethernet_writePHYRegister(EMAC_BASE, 0x0DU, 0x401FU);  /* Data mode */
        uint16_t uiLedCfg1 = Ethernet_readPHYRegister(EMAC_BASE, 0x0EU);
        
        uiLedCfg1 &= ~(0x0F00U); /* LED_1 Control (Bits [11:8]) 초기화 */
        Ethernet_writePHYRegister(EMAC_BASE, 0x0EU, uiLedCfg1);

        // 3. LEDCFG2 (0x0469) 레지스터 설정: LED_1 오버라이드 및 강제 점등
        Ethernet_writePHYRegister(EMAC_BASE, 0x0DU, 0x001FU);  /* Address mode */
        Ethernet_writePHYRegister(EMAC_BASE, 0x0EU, 0x0469U);  /* ADDAR = 0x0469 (LEDCFG2) */
        Ethernet_writePHYRegister(EMAC_BASE, 0x0DU, 0x401FU);  /* Data mode */
        uint16_t uiLedCfg2 = Ethernet_readPHYRegister(EMAC_BASE, 0x0EU);
        
        /* * [DP83822 LED_1 하드웨어 제어 비트 구조 수정]
         * Bit 4: LED_1 Force Override Enable
         * Bit 5: LED_1 Force Value
         * Bit 6: LED_1 Polarity (0 = Active Low, 1 = Active High)
         */
        uiLedCfg2 &= ~(0x0070U); /* LED_1 제어 비트 영역 [6:4] Clear */
        
        /* * Active Low(Bit 6 = 0) 구조에서 LED를 켜려면 핀 출력이 Low가 되어야 하므로 
         * Force Value(Bit 5)를 0으로 주거나, Active High 구조로 변환 후 조작해야 안전합니다.
         * 여기서는 명확하게 핀을 Low 상태로 드라이브하기 위해 비트를 조립합니다.
         */
        uiLedCfg2 |= 0x0010U;    /* Override Enable(Bit 4) = 1, Value(Bit 5) = 0, Polarity(Bit 6) = 0 (Active Low) */
        Ethernet_writePHYRegister(EMAC_BASE, 0x0EU, uiLedCfg2);

        /* --- [Phase 4] EMAC RX 하드웨어 인터럽트 등록 및 활성화 --- */
        Interrupt_registerHandler(INT_EMAC_RX0, isr_EmacRx0);
        Interrupt_enable(INT_EMAC_RX0);
    }
}

/* ---------------------------------------------------------------
 * 이더넷 수신 폴링 태스크 (main loop 에서 호출)
 * --------------------------------------------------------------- */
/*
@function    updateEthernetTask
@brief      이더넷 수신 완료 큐를 폴링하여 수신된 패킷을 처리합니다. (ISR에서 호출)
@param      void
@return     void
@remark
    - EMAC 핸들이 유효한 경우에만 동작합니다.
    - 수신된 패킷은 App_ethRxCallback 콜백을 통해 처리됩니다.
*/
void updateEthernetTask(void)
{
    if (xEthDriver.hEMAC != (Ethernet_Handle)0U)
    {
        /* 수신된 패킷 처리 (DMA CP 레지스터 업데이트를 통해 인터럽트 소스 해제) */
        Ethernet_removePacketsFromRxQueue(&((Ethernet_Device *)xEthDriver.hEMAC)->dmaObj.rxDma[0U], ETHERNET_COMPLETION_NORMAL);
        
        /* 송신 완료된 패킷의 디스크립터를 큐에서 제거하여 풀 방지 */
        Ethernet_removePacketsFromTxQueue(&((Ethernet_Device *)xEthDriver.hEMAC)->dmaObj.txDma[0U], ETHERNET_COMPLETION_NORMAL);
    }
}

/* ---------------------------------------------------------------
 * 이더넷 수신 하드웨어 인터럽트 (Phase 4)
 * --------------------------------------------------------------- */
/*
@function    isr_EmacRx0
@brief      EMAC RX0 수신 하드웨어 인터럽트 ISR
@param      void
@return     static void
@remark
    - 하드웨어 이더넷 수신 완료 시 호출되어 updateEthernetTask를 기동합니다.
*/
static void isr_EmacRx0(void)
{
    updateEthernetTask();

    /* [버그 수정] 하드웨어 인터럽트 상태 명시적 클리어 */
    /* 수신 인터럽트(RI) 및 일반 인터럽트 요약(NIS) 비트를 해제하지 않으면 무한 인터럽트에 빠져 메인 루프가 멈춥니다. */
    if (xEthDriver.hEMAC != (Ethernet_Handle)0U)
    {
        Ethernet_clearDMAChannelInterrupt(
            EMAC_BASE,
            ETHERNET_DMA_CHANNEL_NUM_0, 
            ETHERNET_DMA_CH0_STATUS_RI | ETHERNET_DMA_CH0_STATUS_NIS);
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
    /* xEthDriver.txBuf 는 정적 배열이므로 별도 해제 불필요 */
}
