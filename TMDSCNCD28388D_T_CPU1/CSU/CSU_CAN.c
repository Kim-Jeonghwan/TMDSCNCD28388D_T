/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_CAN.c
    Description      : CAN Communication Application Layer
                       - HW 초기화는 DevCan의 InitialCan() 에 위임
                       - Rx ISR: CanaISR()에서 rxMsgData1[] 저장 + bCanRxFlag 세트 → recvCanMessage() 파싱
                       - 10ms 주기: recvCanMessage() 수신 + sendCanMessage() 송신 + updateCanStatus() 상태 체크
    Last Updated     : 2026. 05. 29. (정적 시험 기준 준수 및 단일 Exit Point 리팩토링)
**********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "CSU_CAN.h"
#include "CSU_SCI_PC.h"
#include "CSU_LED.h"


/* ************************** [[   global   ]]  *********************************************************** */
stRcvCanMsg     xRcvCanMsg;
stXmtCanMsg     xXmtCanMsg;
eCanState       xCanState   = eCAN_STATE_UNINIT;

/* ISR 수신 완료 플래그 */
bool            bCanRxFlag  = false;


/* ************************** [[  static prototype  ]]  *************************************************** */
static void clearRcvMsg(void);


/* ************************** [[  function  ]]  *********************************************************** */

/**
 * @brief 수신 구조체 초기화
 */
static void clearRcvMsg(void)
{
    xRcvCanMsg.IncNumber        = 0u;
    xRcvCanMsg.LedData          = 0u;
}


/**
 * @brief CAN 모듈 초기화
 * @details
 *   1) DevCan의 InitialCan() 호출 → GPIO70/71, 1Mbps, Extended, ISR, MsgObj 설정
 *   2) CSU 레벨 구조체 클리어
 *   DevDspInit.c의 InitialPeripherals() 에서 호출
 */
void Initial_CAN(void)
{
    /* 1. HW 초기화: GPIO, Bitrate, MsgObj, ISR 등록 모두 DevCan이 담당 */
    InitialCan();

    /* 2. CSU 구조체 초기화 */
    clearRcvMsg();

    xXmtCanMsg.IncNumber        = 0u;
    xXmtCanMsg.Status           = 0u;

    bCanRxFlag  = false;
    xCanState   = eCAN_STATE_READY;
}


/**
 * @brief CAN Rx ISR 플래그 기반 수신 파싱 (10ms 주기)
 * @details
 *   CanaISR()에서 bCanRxFlag = true 세트 후 rxMsgData1[]에 데이터 저장
 *   → 이 함수에서 플래그 확인 후 파싱 (SCI 방식과 동일)
 *
 *   수신 데이터 포맷 (Extended Frame, 8 Bytes):
 *     Byte[0] : IncNumber
 *     Byte[1] : Command 비트필드
 *     Byte[2] : EepAddr Low
 *     Byte[3] : EepAddr High
 *     Byte[4] : EepromWriteVal
 *     Byte[5] : PWM Duty (0~100)
 *     Byte[6] : PWM Freq 코드
 *     Byte[7] : Reserved
 */
void recvCanMessage(void)
{
    if(xCanState != eCAN_STATE_UNINIT)
    {
        /* ISR 수신 완료 플래그 확인 (CanaISR에서 세트) */
        if(bCanRxFlag == true)
        {
            /* 2. 파싱 및 구조체 업데이트 (LED 데이터만 취급) */
            xRcvCanMsg.IncNumber        = (uint16_t)(rxMsgData1[0u] & 0xFFu);
            xRcvCanMsg.LedData          = (uint16_t)(rxMsgData1[1u] & 0xFFu);

            /* --- [ 기존 제어 모듈과의 동기화 ] --- */
            /* LED 제어 모듈은 xRcvSciPcMsg1.Command를 참조하므로 데이터를 복사함 */
            xRcvSciPcMsg1.IncNumber     = xRcvCanMsg.IncNumber;
            xRcvSciPcMsg1.Command.all    = xRcvCanMsg.LedData; // 하위 8비트가 LED 상태임

            /* 실제 LED 제어 함수 호출 */
            updateLedStatus();

            /* 처리 완료 후 플래그 클리어 */
            xCanState  = eCAN_STATE_RX_OK;
            bCanRxFlag = false;
        }
    }
}


/**
 * @brief CAN Tx 송신 (10ms 주기)
 * @details xXmtCanMsg 구조체를 8바이트로 직렬화하여
 *          DevCan의 SendCanaMessage() 로 전송합니다.
 *
 *   송신 데이터 포맷 (Extended Frame, 8 Bytes):
 *     Byte[0] : IncNumber (자동 증가, 0~255)
 *     Byte[1] : Status (b0=Tact01, b1=Tact02)
 *     Byte[2] : PotenRAW Low
 *     Byte[3] : PotenRAW High
 *     Byte[4] : PWMRaw Low
 *     Byte[5] : PWMRaw High
 *     Byte[6] : EepromReadVal
 *     Byte[7] : Reserved
 */
void sendCanMessage(void)
{
    uint16_t    txData[MSG_DATA_LENGTH];
    uint16_t    i;

    if(xCanState == eCAN_STATE_UNINIT || xCanState == eCAN_STATE_BUS_OFF)
    {
        return;
    }

    /* 8바이트 버퍼 초기화 */
    for(i=0; i<MSG_DATA_LENGTH; i++) txData[i] = 0;

    /* Byte[0] : IncNumber (자동 증가) */
    txData[0u] = (uint16_t)(xXmtCanMsg.IncNumber++ & 0xFFu);

    /* Byte[1] : Status (스위치 상태) */
    txData[1u] = (uint16_t)(xXmtCanMsg.Status & 0xFFu);

    /* 나머지 바이트는 0으로 전송 (단순화) */

    /* ID 0x15555555 한 개만 송신 */
    SendCanaMessage(TX_MSG_OBJ_NO1, MSG_DATA_LENGTH, (Uint16 *)txData);

    xCanState = eCAN_STATE_TX_PENDING;
}


/**
 * @brief CAN 모듈 상태 모니터링 (10ms 주기)
 * @details CAN_getStatus() 로 ES 레지스터를 읽어 Bus-Off 여부 확인
 *          Bus-Off 감지 시 CAN_startModule() 로 복구 시도
 */
void updateCanStatus(void)
{
    uint32_t    esStatus;

    if(xCanState != eCAN_STATE_UNINIT)
    {
        /* CAN 상태(Error & Status) 레지스터 읽기 (읽는 순간 TXOK, RXOK 비트 클리어됨) */
        esStatus = CAN_getStatus(CSU_CAN_BASE);

        /* 1. Bus-Off 확인 (최우선) */
        if((esStatus & (uint32_t)CAN_STATUS_BUS_OFF) != 0u)
        {
            xCanState = eCAN_STATE_BUS_OFF;
            
            /* 복구 시도: 모듈 재시작 */
            CAN_startModule(CSU_CAN_BASE);
        }
        /* 2. 에러 상태 확인 (EWARN, EPASS 등) */
        else if((esStatus & ((uint32_t)CAN_STATUS_EWARN | (uint32_t)CAN_STATUS_EPASS)) != 0u)
        {
            xCanState = eCAN_STATE_ERROR;
        }
        /* 3. 정상 동작 상태 업데이트 */
        else
        {
            if((esStatus & (uint32_t)CAN_STATUS_RXOK) != 0u)
            {
                /* 최근에 수신 성공함 */
                xCanState = eCAN_STATE_RX_OK;
            }
            else if((esStatus & (uint32_t)CAN_STATUS_TXOK) != 0u)
            {
                /* 최근에 송신 성공함 */
                xCanState = eCAN_STATE_READY;
            }
            else
            {
                /* 아무 비트도 뜨지 않았다면 이전 상태(TX_PENDING 등) 유지 */
            }
        }
    }
}


/**
 * @brief 실시간 센서 데이터를 CAN 송신 구조체에 반영 (10ms 주기)
 * @details SCI_PC용으로 업데이트된 최신 데이터를 CAN 패킷용 구조체로 복사합니다.
 */
void updateCanXmtData(void)
{
    /* SCI_PC 구조체에 담긴 비트 필드들을 합쳐서 Status 생성 */
    xXmtCanMsg.Status           = (uint16_t)((uint16_t)xXmtSciPcMsg1.Tact01 | 
                                            ((uint16_t)xXmtSciPcMsg1.Tact02 << 1u));
}
