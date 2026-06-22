/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Ethernet.c
    Version          : 00.05
    Description      : UDP 프로토콜 처리 - Payload/ACK MSG 조립/파싱
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 22. (수신 데이터 추출 로직 분기 이동하여 파형 변경 반영 오류 수정)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 22. - 수신 데이터 추출 로직 분기 이동하여 파형 변경 반영 오류 수정
 * 2026. 06. 22. - 물리 파일명을 csu_Ethernet.c로 소문자 정정 및 GSRAM 잔재 주석 수정
 * 2026. 06. 19. - 변수명 규칙 적용 (xCsuEth, xHalEth -> xEthApp, xEthDriver 변경)
 * 2026. 06. 19. - 이더넷 전역 변수 캡슐화 적용
 * 2026. 06. 19. - Phase 5: 사인파 추가 및 PC 요청 응답 구조 변경
 * 2026. 06. 22. - IPC 전송(sendIpcMessageToCPU1) 제거 및 MSGRAM Seqlock 동기화 쓰기(CMTOCPU1)/읽기(CPU1TOCM) 적용
 */

/*
 * [패킷 구조]
 * DSP→PC Reflect (2ms):
 *   ETH(14B) + IP(20B) + UDP(8B) + Payload(19B) = 61B
 *   Payload: Header(12B) + Data(4B) + Checksum(2B) + Timestamp(4B 포함)
 *
 * PC→DSP Update (10ms):
 *   ETH(14B) + IP(20B) + UDP(8B) + Payload(17B) = 59B
 *   Payload: Header(12B) + Data(2B) + Checksum(2B)
 *
 * DSP→PC ACK (Update 수신 즉시):
 *   ETH(14B) + IP(20B) + UDP(8B) + Payload(18B) = 60B
 *   Payload: Header(12B) + Data(4B) + Checksum(2B)
 *
 * [Checksum 규칙]
 *   Checksum 제외 전체 페이로드 바이트 합산 최하위 2바이트 (Little Endian)
 *
 * [Endian] 모든 다중 바이트 필드는 Little Endian
 */

#include "csu_Ethernet.h"

/* 전송용 패킷 디스크립터 구조체 (Persistent 유지) */
/* TX 패킷 디스크립터 풀 (단일 변수 사용 시 큐 꼬임 방지) */
#define ETH_TX_NUM_PKT_DESC  (4U)
static Ethernet_Pkt_Desc s_xTxPktDesc[ETH_TX_NUM_PKT_DESC];
static uint8_t s_ucTxPktDescIdx = 0U;


/* ---------------------------------------------------------------
 * IP 헤더 오프셋 상수 (이더넷 프레임 기준)
 * --------------------------------------------------------------- */
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

/* 최대 프레임 크기 */
#define TX_REFLECT_FRAME_SIZE  (64U) /* ETH(14) + IP(20) + UDP(8) + Payload(22) */
#define TX_ACK_FRAME_SIZE      (60U) /* ETH + IP + UDP + 18B payload */

/* 최소 수신 프레임 크기 (유효성 검사) */
#define MIN_RX_FRAME_SIZE      (ETH_HDR_SIZE + IP_HDR_SIZE + UDP_HDR_SIZE + ETH_MSG_HEADER_SIZE + ETH_CHECKSUM_SIZE)

/* ---------------------------------------------------------------
 * 공유 데이터 전역 변수 (csu_Ipc_cm.c 에서 갱신)
 * --------------------------------------------------------------- */
stEthAppState xEthApp = {
    .txData = {0U, 0U, 0U},
    .rxData = {0U, 0U, 0U},
    .realPcMac = {ETH_PC_MAC0, ETH_PC_MAC1, ETH_PC_MAC2, ETH_PC_MAC3, ETH_PC_MAC4, ETH_PC_MAC5},
    .lastRxSrcPort = 0U
};

/* 이더넷 활동(Tx/Rx) LED 표시용 타이머 (1ms 주기 감소) */
uint16_t ethActivityTimer = 0U;

#define ETH_LED_PIN 146U
#define ETH_LED_ON()  GPIO_writePin(ETH_LED_PIN, 1U)

/* ---------------------------------------------------------------
 * static 함수 선언
 * --------------------------------------------------------------- */
static uint16_t calcUdpMsgChecksum(const uint8_t *pBuf, uint16_t length);
static uint16_t calcIPChecksum(const uint8_t *pIPHdr);
static void     buildEthernetHeader(uint8_t *pFrame);
static void     buildIPHeader(uint8_t *pFrame, uint16_t udpPayloadLen);
static void     buildUDPHeader(uint8_t *pFrame, uint16_t payloadLen);
static bool     sendEthernetFrame(uint8_t *pFrame, uint16_t frameSize);

/* ---------------------------------------------------------------
 * IP 헤더 체크섬 계산 (RFC791)
 * --------------------------------------------------------------- */
/*
@funtion    static uint16_t calcIPChecksum(const uint8_t *pIPHdr)
@brief      IP 헤더 20바이트의 RFC791 방식 체크섬을 계산합니다.
@param      pIPHdr: IP 헤더 시작 포인터 (20B)
@return     uint16_t: 계산된 IP 체크섬 (네트워크 바이트 순서)
@remark
    - CWE-476: NULL 체크 후 사용
*/
static uint16_t calcIPChecksum(const uint8_t *pIPHdr)
{
    uint32_t uiSum   = 0U;
    uint16_t uiWord  = 0U;
    uint8_t   i       = 0U;
    uint16_t uiRet   = 0U;

    if (pIPHdr == NULL)
    {
        uiRet = 0U;
    }
    else
    {
        /* 16비트 단위로 합산 (20B = 10 words) */
        for (i = 0U; i < IP_HDR_SIZE; i += 2U)
        {
            uiWord = ((uint16_t)pIPHdr[i] << 8U) | (uint16_t)pIPHdr[i + 1U];
            uiSum += (uint32_t)uiWord;
        }

        /* 캐리 접기 */
        while ((uiSum >> 16U) != 0U)
        {
            uiSum = (uiSum & 0x0000FFFFU) + (uiSum >> 16U);
        }

        uiRet = (uint16_t)(~uiSum);
    }

    return uiRet;
}

/* ---------------------------------------------------------------
 * UDP 메시지 Payload 체크섬 계산 (규격서: 최하위 2바이트)
 * --------------------------------------------------------------- */
/*
@funtion    static uint16_t calcUdpMsgChecksum(const uint8_t *pBuf, uint16_t length)
@brief      규격서에 정의된 Payload Checksum 계산 (전체 바이트 합 최하위 2바이트)
@param      pBuf   : Checksum 필드 제외 대상 버퍼 포인터
@param      length : 계산 대상 바이트 수
@return     uint16_t: 최하위 2바이트 합산값 (Little Endian 저장용)
@remark
    - CWE-476: NULL 체크 수행
*/
static uint16_t calcUdpMsgChecksum(const uint8_t *pBuf, uint16_t length)
{
    uint32_t uiSum  = 0U;
    uint16_t i      = 0U;
    uint16_t uiRet  = 0U;

    if (pBuf == NULL)
    {
        uiRet = 0U;
    }
    else
    {
        for (i = 0U; i < length; i++)
        {
            uiSum += (uint32_t)pBuf[i];
        }
        uiRet = (uint16_t)(uiSum & 0x0000FFFFU);
    }

    return uiRet;
}

/*
@funtion    static void buildEthernetHeader(uint8_t *pFrame)
@brief      이더넷 헤더 조립 (DSP→PC 방향)
@param      pFrame: 프레임 버퍼 포인터
@return     static void
@remark
    - 동적으로 캡처된 PC의 MAC 주소를 목적지로 설정합니다.
*/
static void buildEthernetHeader(uint8_t *pFrame)
{
    /* Destination MAC: 동적으로 캡처된 PC MAC */
    pFrame[0U] = xEthApp.realPcMac[0];
    pFrame[1U] = xEthApp.realPcMac[1];
    pFrame[2U] = xEthApp.realPcMac[2];
    pFrame[3U] = xEthApp.realPcMac[3];
    pFrame[4U] = xEthApp.realPcMac[4];
    pFrame[5U] = xEthApp.realPcMac[5];

    /* 출발지 MAC: DSP MAC A8:63:F2:00:38:88 */
    pFrame[6U]  = ETH_DSP_MAC0;
    pFrame[7U]  = ETH_DSP_MAC1;
    pFrame[8U]  = ETH_DSP_MAC2;
    pFrame[9U]  = ETH_DSP_MAC3;
    pFrame[10U] = ETH_DSP_MAC4;
    pFrame[11U] = ETH_DSP_MAC5;

    /* EtherType: IPv4 (0x0800) Big Endian */
    pFrame[12U] = 0x08U;
    pFrame[13U] = 0x00U;
}

/*
@funtion    static void buildIPHeader(uint8_t *pFrame, uint16_t udpPayloadLen)
@brief      IP 헤더 조립 (DSP→PC 방향, Big Endian)
@param      pFrame: 프레임 버퍼 포인터
@param      udpPayloadLen: UDP 페이로드 바이트 수
@return     static void
@remark
    - IPv4 규격에 따라 IP 헤더 20바이트를 구성합니다.
*/
static void buildIPHeader(uint8_t *pFrame, uint16_t udpPayloadLen)
{
    uint8_t  *pIP       = pFrame + ETH_HDR_SIZE;
    uint16_t  uiTotalLen = (uint16_t)IP_HDR_SIZE + (uint16_t)UDP_HDR_SIZE + udpPayloadLen;
    uint16_t  uiChksum   = 0U;

    /* Version=4, IHL=5 */
    pIP[0U]  = IP_HDR_VER_IHL;
    /* DSCP/ECN */
    pIP[1U]  = IP_HDR_DSCP;
    /* Total Length (Big Endian) */
    pIP[2U]  = (uint8_t)(uiTotalLen >> 8U);
    pIP[3U]  = (uint8_t)(uiTotalLen & 0x00FFU);
    /* Identification: 0 */
    pIP[4U]  = 0x00U;
    pIP[5U]  = 0x00U;
    /* Flags + Fragment Offset: Don't Fragment */
    pIP[6U]  = 0x40U;
    pIP[7U]  = 0x00U;
    /* TTL */
    pIP[8U]  = IP_TTL;
    /* Protocol: UDP */
    pIP[9U]  = IP_PROTO_UDP;
    /* Header Checksum: 먼저 0으로 클리어 후 계산 */
    pIP[10U] = 0x00U;
    pIP[11U] = 0x00U;
    /* Source IP: 192.168.100.10 */
    pIP[12U] = ETH_DSP_IP0;
    pIP[13U] = ETH_DSP_IP1;
    pIP[14U] = ETH_DSP_IP2;
    pIP[15U] = ETH_DSP_IP3;
    /* Destination IP: 192.168.100.100 */
    pIP[16U] = ETH_PC_IP0;
    pIP[17U] = ETH_PC_IP1;
    pIP[18U] = ETH_PC_IP2;
    pIP[19U] = ETH_PC_IP3;

    /* IP 헤더 체크섬 계산 후 삽입 (Big Endian) */
    uiChksum = calcIPChecksum(pIP);
    pIP[10U] = (uint8_t)(uiChksum >> 8U);
    pIP[11U] = (uint8_t)(uiChksum & 0x00FFU);
}

/*
@funtion    static void buildUDPHeader(uint8_t *pFrame, uint16_t payloadLen)
@brief      UDP 헤더 조립 (DSP→PC 방향, Big Endian)
@param      pFrame: 프레임 버퍼 포인터
@param      payloadLen: UDP 페이로드 바이트 수
@return     static void
@remark
    - UDP 포트를 할당하며 수신된 포트(동적 캡처)를 활용합니다.
*/
static void buildUDPHeader(uint8_t *pFrame, uint16_t payloadLen)
{
    uint8_t  *pUDP     = pFrame + UDP_HDR_OFFSET;
    uint16_t  uiUdpLen = (uint16_t)UDP_HDR_SIZE + payloadLen;

    /* Source Port: DSP RX Port (5001, Big Endian) */
    pUDP[0U] = (uint8_t)(ETH_DSP_RX_PORT >> 8U);
    pUDP[1U] = (uint8_t)(ETH_DSP_RX_PORT & 0x00FFU);
    
    /* Destination Port: PC RX Port (수신된 포트가 있으면 동적으로 사용, 없으면 기본값) */
    uint16_t pcDestPort = (xEthApp.lastRxSrcPort != 0U) ? xEthApp.lastRxSrcPort : ETH_PC_RX_PORT;
    pUDP[2U] = (uint8_t)(pcDestPort >> 8U);
    pUDP[3U] = (uint8_t)(pcDestPort & 0x00FFU);
    
    /* Length (Big Endian) */
    pUDP[4U] = (uint8_t)(uiUdpLen >> 8U);
    pUDP[5U] = (uint8_t)(uiUdpLen & 0x00FFU);
    /* UDP Checksum: 0 (비활성) */
    pUDP[6U] = 0x00U;
    pUDP[7U] = 0x00U;
}

/* ---------------------------------------------------------------
 * EMAC 드라이버로 이더넷 프레임 전송
 * --------------------------------------------------------------- */
/*
@funtion    static bool sendEthernetFrame(uint8_t *pFrame, uint16_t frameSize)
@brief      조립된 이더넷 프레임을 EMAC 드라이버의 Ethernet_sendPacket 으로 전송합니다.
@param      pFrame   : 전송할 프레임 버퍼 포인터
@param      frameSize: 전송 바이트 수
@return     bool: true=전송 요청 성공, false=핸들 없음 또는 전송 실패
@remark
    - CWE-476: g_hEMAC NULL 체크 수행
*/
static bool sendEthernetFrame(uint8_t *pFrame, uint16_t frameSize)
{
    bool bRet = false;

    if ((xEthDriver.hEMAC != (Ethernet_Handle)0U) && (pFrame != NULL))
    {
        Ethernet_Pkt_Desc *pTxDesc = &s_xTxPktDesc[s_ucTxPktDescIdx];

        pTxDesc->dataBuffer     = pFrame;
        pTxDesc->dataOffset     = 0U;
        pTxDesc->validLength    = (uint32_t)frameSize;
        pTxDesc->bufferLength   = (uint32_t)frameSize;
        pTxDesc->pktLength      = (uint32_t)frameSize;
        pTxDesc->pktChannel     = ETHERNET_DMA_CHANNEL_NUM_0;
        pTxDesc->numPktFrags    = 1U;
        /* SOP|EOP 필수: TI EMAC 드라이버는 SOP 비트 없으면 즉시 거부 (ethernet.c 참조) */
        pTxDesc->flags          = ETHERNET_PKT_FLAG_SOP | ETHERNET_PKT_FLAG_EOP;
        pTxDesc->nextPacketDesc = NULL;

        uint32_t myErr = 0U;
        if ((pTxDesc->flags & ETHERNET_PKT_FLAG_SOP) == 0U) { myErr = 1U; }
        else if (pTxDesc->pktLength > 1536U) { myErr = 2U; }
        else if (pTxDesc->nextPacketDesc != NULL) { myErr = 3U; }
        else if (pTxDesc->numPktFrags != 1U) { myErr = 4U; }
        else if (((Ethernet_Device*)xEthDriver.hEMAC)->dmaObj.txDma[0].descMax < 1U) { myErr = 5U; }
        
        uint32_t retCode = Ethernet_sendPacket(xEthDriver.hEMAC, pTxDesc);
        
        if ((myErr == 0U) && (retCode == 0U))
        {
            /* 인덱스 순환 증가 (다음 송신 시 새로운 디스크립터 사용) */
            s_ucTxPktDescIdx = (s_ucTxPktDescIdx + 1U) % ETH_TX_NUM_PKT_DESC;
            bRet = true;
        }
        else
        {
            /* TX 실패 시 예외 처리 (재전송 등은 상위 계층에서 담당) */
        }
    }
    else
    {
        /* 핸들 무효 또는 NULL 포인터 */
    }

    return bRet;
}

/* ---------------------------------------------------------------
 * DSP→PC Reflect 패킷 조립 및 송신
 * --------------------------------------------------------------- */
/*
@function    buildAndSendUdpPacket
@brief      Reflect 타입 UDP 패킷(온도+시퀀스+사인파)을 조립하여 송신합니다.
@param      uint32_t rxTimestamp: 요청 패킷의 Timestamp (응답 시 반환)
@return     void
@remark
    - CPU1이 CPU1TOCM MSGRAM에 기록한 데이터를 Seqlock 방식으로 읽어옵니다.
    - Payload: MSG Header(12B) + Data(8B) + Checksum(2B) = 22B
    - Request Ack = 0xFF (Reflect 타입, ACK 미요청)
*/
void buildAndSendUdpPacket(uint32_t rxTimestamp)
{
    uint8_t  *pPayload   = xEthDriver.txBuf + PAYLOAD_OFFSET;
    uint16_t  uiChksum   = 0U;
    uint16_t  uiChksumLen = 0U;
    uint32_t  sineBits   = 0U;

    float32_t tempWave = 0.0f;
    float32_t tempTemp = 0.0f;
    uint32_t  tempSeq  = 0U;
    uint32_t  seq0, seq1;

    /* ---- CPU1TOCM MSGRAM 데이터 읽기 (Seqlock Spin-Lock 검증) ---- */
    do {
        seq0 = pxDataCpu1ToCm->seqCount;
        if ((seq0 & 1U) != 0U) { continue; } /* 홀수면 CPU1이 쓰기 중이므로 대기 */
        
        /* 데이터 복사 */
        tempWave = pxDataCpu1ToCm->Payload.TxData.waveValue;
        tempTemp = pxDataCpu1ToCm->Payload.TxData.adcTemperature;
        tempSeq  = pxDataCpu1ToCm->Payload.TxData.sequenceNum;
        
        seq1 = pxDataCpu1ToCm->seqCount;
    } while (seq0 != seq1); /* 읽는 도중 값이 변경되었다면 다시 읽기 */

    /* ---- MSG Header (12B) ---- */
    /* Timestamp (4B, Little Endian) */
    pPayload[0U]  = (uint8_t)(rxTimestamp & 0x000000FFU);
    pPayload[1U]  = (uint8_t)((rxTimestamp >>  8U) & 0x000000FFU);
    pPayload[2U]  = (uint8_t)((rxTimestamp >> 16U) & 0x000000FFU);
    pPayload[3U]  = (uint8_t)((rxTimestamp >> 24U) & 0x000000FFU);
    /* MSG ID */
    pPayload[4U]  = ETH_SRC_ID_DSP;          /* Source ID */
    pPayload[5U]  = ETH_DST_ID_PC;           /* Destination ID */
    pPayload[6U]  = ETH_MSG_CODE_MONITOR;    /* Code */
    pPayload[7U]  = ETH_REQ_ACK_NONE;        /* Request Ack: 0xFF (Reflect) */
    pPayload[8U]  = ETH_PRIORITY_NORMAL;     /* Priority */
    pPayload[9U]  = ETH_SEND_COUNT_INIT;     /* Send Count */
    /* Data Length (2B, Little Endian) = 8 */
    pPayload[10U] = (uint8_t)(ETH_PAYLOAD_DATA_SIZE & 0x00FFU);
    pPayload[11U] = (uint8_t)(ETH_PAYLOAD_DATA_SIZE >> 8U);

    /* ---- Data (8B) ---- */
    pPayload[12U] = (uint8_t)(tempSeq & 0xFFU);
    pPayload[13U] = 0U; /* Status: 기본값 0 */
    /* DspTemp (2B, Little Endian) */
    uint16_t dspTempUint16 = (uint16_t)((tempTemp * 10.0f) + 0.5f);
    pPayload[14U] = (uint8_t)(dspTempUint16 & 0x00FFU);
    pPayload[15U] = (uint8_t)(dspTempUint16 >> 8U);
    /* WaveVal (4B, Little Endian) */
    (void)memcpy(&sineBits, &tempWave, sizeof(uint32_t));
    pPayload[16U] = (uint8_t)(sineBits & 0x000000FFU);
    pPayload[17U] = (uint8_t)((sineBits >>  8U) & 0x000000FFU);
    pPayload[18U] = (uint8_t)((sineBits >> 16U) & 0x000000FFU);
    pPayload[19U] = (uint8_t)((sineBits >> 24U) & 0x000000FFU);

    /* ---- Checksum (2B, Little Endian): 앞 20B 합산 최하위 2B ---- */
    uiChksumLen = ETH_MSG_HEADER_SIZE + ETH_PAYLOAD_DATA_SIZE; /* 20B */
    uiChksum    = calcUdpMsgChecksum(pPayload, uiChksumLen);
    pPayload[20U] = (uint8_t)(uiChksum & 0x00FFU);
    pPayload[21U] = (uint8_t)(uiChksum >> 8U);

    /* ---- 이더넷/IP/UDP 헤더 조립 ---- */
    buildEthernetHeader(xEthDriver.txBuf);
    buildIPHeader(xEthDriver.txBuf, (uint16_t)(ETH_MSG_HEADER_SIZE + ETH_PAYLOAD_DATA_SIZE + ETH_CHECKSUM_SIZE));
    buildUDPHeader(xEthDriver.txBuf, (uint16_t)(ETH_MSG_HEADER_SIZE + ETH_PAYLOAD_DATA_SIZE + ETH_CHECKSUM_SIZE));

    /* Tx 활동 LED 점등 (타이머 리셋) */
    ETH_LED_ON();
    ethActivityTimer = 20U; /* 20ms 유지 */

    /* ---- 전송 ---- */
    (void)sendEthernetFrame(xEthDriver.txBuf, (uint16_t)TX_REFLECT_FRAME_SIZE);
}

/* ---------------------------------------------------------------
 * PC→DSP Update 패킷 수신 파싱 및 ACK 응답
 * --------------------------------------------------------------- */
/*
@funtion    void processReceivedEthernetPacket(uint8_t *pPacket, uint16_t length)
@brief      수신된 이더넷 프레임을 파싱하여 UDP 페이로드 검증 후 CPU1에 IPC 전달합니다.
@param      pPacket: 수신된 이더넷 프레임 버퍼 포인터
@param      length : 수신 바이트 수
@return     void
@remark
    - CWE-476: pPacket NULL 체크 수행
    - Checksum 검증: 실패 시 NACK 응답, 성공 시 ACK 응답 후 CPU1 IPC 전달
    - UDP 수신 포트 5001 필터링, Source ID 0x20(PC) 필터링
*/
void processReceivedEthernetPacket(uint8_t *pPacket, uint16_t length)
{
    uint8_t  *pPayload   = NULL;
    uint16_t  uiRxCalcChk  = 0U;
    uint16_t  uiRxRecvChk  = 0U;
    uint16_t  uiPayloadChkLen = 0U;
    uint16_t  uiDstPort    = 0U;
    uint32_t  uiTimestamp = 0U;
    uint8_t   ucCode      = 0U;

    if ((pPacket != NULL) && (length >= (uint16_t)MIN_RX_FRAME_SIZE))
    {
        /* ---- 패킷 타입 확인 (IPv4 또는 ARP) ---- */
        uint16_t ethType = ((uint16_t)pPacket[ETH_HDR_TYPE_OFFSET] << 8U) | (uint16_t)pPacket[ETH_HDR_TYPE_OFFSET + 1U];
        
        if (ethType == 0x0806U) /* ARP */
        {
            /* ARP Request(Opcode 1)인지 확인 */
            if ((pPacket[20U] == 0x00U) && (pPacket[21U] == 0x01U))
            {
                /* 수신된 ARP 요청의 Target IP가 DSP IP(192.168.100.10)와 일치하는지 확인 */
                if ((pPacket[38U] == ETH_DSP_IP0) && (pPacket[39U] == ETH_DSP_IP1) &&
                    (pPacket[40U] == ETH_DSP_IP2) && (pPacket[41U] == ETH_DSP_IP3))
                {
                    /* 당사 IP를 찾는 유효한 ARP 요청일 경우 Rx 활동 LED 점등 */
                    ETH_LED_ON();
                    ethActivityTimer = 20U;

                    /* ---- ARP Reply 조립 (DMA 비동기 전송을 위해 반드시 static 선언 필요!) ---- */
                    static uint8_t arpReply[60] = {0};
                    uint16_t i;
                    
                    /* 1. Ethernet Header (Dst MAC = Sender MAC from ARP) */
                    for(i=0; i<6; i++) arpReply[i] = pPacket[22U + i];
                    /* Src MAC = DSP MAC */
                    arpReply[6] = ETH_DSP_MAC0; arpReply[7] = ETH_DSP_MAC1; arpReply[8] = ETH_DSP_MAC2;
                    arpReply[9] = ETH_DSP_MAC3; arpReply[10] = ETH_DSP_MAC4; arpReply[11] = ETH_DSP_MAC5;
                    arpReply[12] = 0x08U; arpReply[13] = 0x06U; /* Type: ARP */
                    
                    /* 2. ARP Payload */
                    arpReply[14] = 0x00U; arpReply[15] = 0x01U; /* HTYPE: Ethernet */
                    arpReply[16] = 0x08U; arpReply[17] = 0x00U; /* PTYPE: IPv4 */
                    arpReply[18] = 0x06U; arpReply[19] = 0x04U; /* HLEN: 6, PLEN: 4 */
                    arpReply[20] = 0x00U; arpReply[21] = 0x02U; /* Opcode: Reply(2) */
                    
                    /* Sender MAC = DSP MAC */
                    for(i=0; i<6; i++) arpReply[22U + i] = arpReply[6U + i];
                    /* Sender IP = DSP IP */
                    arpReply[28] = ETH_DSP_IP0; arpReply[29] = ETH_DSP_IP1; arpReply[30] = ETH_DSP_IP2; arpReply[31] = ETH_DSP_IP3;
                    
                    /* Target MAC = PC MAC (from request's Sender MAC) */
                    for(i=0; i<6; i++) arpReply[32U + i] = pPacket[22U + i];
                    /* Target IP = PC IP (from request's Sender IP) */
                    for(i=0; i<4; i++) arpReply[38U + i] = pPacket[28U + i];
                    
                    /* ARP 패킷 전송 (길이는 최소 42바이트, 패딩 포함 60바이트 권장) */
                    (void)sendEthernetFrame(arpReply, 60U);
                }
            }
        }
        else if (ethType == 0x0800U) /* IPv4 */
        {
            /* ---- UDP 프로토콜 확인 ---- */
            if (pPacket[IP_HDR_OFFSET + 9U] == IP_PROTO_UDP)
            {
                /* ---- UDP 목적지 포트 확인: 5001 (DSP 수신 포트) ---- */
                uiDstPort = ((uint16_t)pPacket[UDP_HDR_OFFSET + 2U] << 8U) |
                             (uint16_t)pPacket[UDP_HDR_OFFSET + 3U];

                if (uiDstPort == ETH_DSP_RX_PORT)
                {
                    /* 수신된 패킷이 5001번일 경우에만 실제 데이터이므로 캡처 진행 및 LED 점등 */
                    ETH_LED_ON();
                    ethActivityTimer = 20U;

                    xEthApp.lastRxSrcPort = ((uint16_t)pPacket[UDP_HDR_OFFSET] << 8U) |
                                         (uint16_t)pPacket[UDP_HDR_OFFSET + 1U];

                    /* PC의 실제 MAC 주소를 캡처하여 저장 (ACK 및 Reflect 시 사용) */
                    uint16_t m;
                    for(m=0; m<6; m++) {
                        xEthApp.realPcMac[m] = pPacket[6U + m]; /* Ethernet Header Src MAC */
                    }

                    pPayload = pPacket + PAYLOAD_OFFSET;

                    /* 페이로드 유효성 검사 (최소 요구사항 충족 시 처리) */
                    if (pPayload[4U] == ETH_DST_ID_PC)
                    {
                        /* ---- Checksum 검증 ---- */
                        ucCode = pPayload[6U];
                        uiPayloadChkLen = ETH_MSG_HEADER_SIZE + ETH_PC_DATA_SIZE; /* 14B */
                        uiRxCalcChk = calcUdpMsgChecksum(pPayload, uiPayloadChkLen);
                        uiRxRecvChk = ((uint16_t)pPayload[uiPayloadChkLen + 1U] << 8U) |
                                       (uint16_t)pPayload[uiPayloadChkLen];

                        /* Timestamp 추출 (Little Endian, 4B) */
                        uiTimestamp = ((uint32_t)pPayload[3U] << 24U) |
                                      ((uint32_t)pPayload[2U] << 16U) |
                                      ((uint32_t)pPayload[1U] <<  8U) |
                                       (uint32_t)pPayload[0U];

                        if (uiRxCalcChk != uiRxRecvChk)
                        {
                            /* Checksum 오류 → NACK 응답 */
                            sendAckResponse(ETH_ACK_NACK, ETH_ACKINFO_CHECKSUM, uiTimestamp, ucCode);
                        }
                        else
                        {
                            /* ---- 공통: Data 추출 및 CMTOCPU1 MSGRAM 기록 (Seqlock 방식) ---- */
                            /* 정상 수신 패킷이므로 무조건 상태 업데이트 적용 */
                            xEthApp.rxData.SeqNum = pPayload[12U];
                            xEthApp.rxData.WaveType = pPayload[13U];

                            /* CPU1으로 전달할 CMTOCPU1 MSGRAM에 기록 */
                            pxDataCmToCpu1->seqCount++; /* 홀수 (쓰기 시작) */
                            pxDataCmToCpu1->Payload.RxData.seqNum = pPayload[12U];
                            pxDataCmToCpu1->Payload.RxData.waveType = pPayload[13U];
                            pxDataCmToCpu1->seqCount++; /* 짝수 (쓰기 완료) */

                            /* 모니터링 코드 확인 시 실시간 응답 (데이터 전송) */
                            if (ucCode == ETH_MSG_CODE_MONITOR)
                            {
                                buildAndSendUdpPacket(uiTimestamp);
                            }
                            else
                            {
                                /* 정상 수신 업데이트 패킷 → ACK 응답 */
                                sendAckResponse(ETH_ACK_OK, ETH_ACKINFO_OK, uiTimestamp, ucCode);
                            }
                        }
                    }
                }
            }
        }
    }
}

/* ---------------------------------------------------------------
 * DSP→PC ACK/NACK 응답 패킷 송신
 * --------------------------------------------------------------- */
/*
@funtion    void sendAckResponse(uint8_t ackResult, uint16_t ackInfo, uint32_t timestamp, uint8_t targetCode)
@brief      ACK MSG Format(18B Payload)을 조립하여 PC로 즉시 송신합니다.
@param      ackResult  : ETH_ACK_OK(0x10) 또는 ETH_ACK_NACK(0x11)
@param      ackInfo    : ETH_ACKINFO_OK(0x0000) 또는 ETH_ACKINFO_CHECKSUM(0x0001)
@param      timestamp  : 수신 메시지의 Timestamp 그대로 리턴
@param      targetCode : ACK 대상 메시지의 Code 필드값
@return     void
@remark
    - ACK 전용 임시 버퍼 사용 (Reflect 버퍼와 분리)
    - Data Length = 4 (고정, Code Info 2B + Ack Info 2B)
*/
void sendAckResponse(uint8_t ackResult, uint16_t ackInfo,
                     uint32_t timestamp, uint8_t targetCode)
{
    static uint8_t s_ucAckBuf[TX_ACK_FRAME_SIZE]; /* ACK 전용 버퍼 */
    uint8_t        *pPayload  = s_ucAckBuf + PAYLOAD_OFFSET;
    uint16_t        uiChksum  = 0U;

    /* ---- MSG Header (12B) ---- */
    /* Timestamp (Little Endian) */
    pPayload[0U]  = (uint8_t)(timestamp & 0x000000FFU);
    pPayload[1U]  = (uint8_t)((timestamp >>  8U) & 0x000000FFU);
    pPayload[2U]  = (uint8_t)((timestamp >> 16U) & 0x000000FFU);
    pPayload[3U]  = (uint8_t)((timestamp >> 24U) & 0x000000FFU);
    pPayload[4U]  = ETH_SRC_ID_DSP;
    pPayload[5U]  = ETH_DST_ID_PC;
    pPayload[6U]  = ETH_MSG_CODE_ACK;    /* Code: 0xFF */
    pPayload[7U]  = ackResult;           /* 0x10=ACK / 0x11=NACK */
    pPayload[8U]  = ETH_PRIORITY_NORMAL;
    pPayload[9U]  = ETH_SEND_COUNT_INIT;
    /* Data Length = 4 (Little Endian) */
    pPayload[10U] = (uint8_t)(ETH_ACK_DATA_SIZE & 0x00FFU);
    pPayload[11U] = (uint8_t)(ETH_ACK_DATA_SIZE >> 8U);

    /* ---- Data (4B): Code Info(2B) + Ack Info(2B) ---- */
    /* Code Info: 수신 메시지 Code (Little Endian) */
    pPayload[12U] = targetCode;
    pPayload[13U] = 0x00U;
    /* Ack Info (Little Endian) */
    pPayload[14U] = (uint8_t)(ackInfo & 0x00FFU);
    pPayload[15U] = (uint8_t)(ackInfo >> 8U);

    /* ---- Checksum (2B, Little Endian): 앞 16B 합산 최하위 2B ---- */
    uiChksum    = calcUdpMsgChecksum(pPayload,
                                     (uint16_t)(ETH_MSG_HEADER_SIZE + ETH_ACK_DATA_SIZE));
    pPayload[16U] = (uint8_t)(uiChksum & 0x00FFU);
    pPayload[17U] = (uint8_t)(uiChksum >> 8U);

    /* ---- 이더넷/IP/UDP 헤더 조립 ---- */
    buildEthernetHeader(s_ucAckBuf);
    buildIPHeader(s_ucAckBuf, (uint16_t)(ETH_MSG_HEADER_SIZE + ETH_ACK_DATA_SIZE + ETH_CHECKSUM_SIZE));
    buildUDPHeader(s_ucAckBuf, (uint16_t)(ETH_MSG_HEADER_SIZE + ETH_ACK_DATA_SIZE + ETH_CHECKSUM_SIZE));

    /* ---- 전송 ---- */
    (void)sendEthernetFrame(s_ucAckBuf, (uint16_t)TX_ACK_FRAME_SIZE);
}
