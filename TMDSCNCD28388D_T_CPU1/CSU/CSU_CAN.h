/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_CAN.h
    Description      : CAN Communication Test Module (Systembase uCAN USB-to-CAN)
                       - CANA 사용: GPIO71(CANTXA), GPIO70(CANRXA)
                       - 비트레이트: 1 Mbps (DevCan 설정 기준)
                       - Extended Frame (29-bit ID)
                       - ISR 기반 수신, 폴링 기반 송신
                       - GPIO/HW 초기화는 DevCan의 InitialCan() 에 위임
    Last Updated     : 2026. 04. 17.
**********************************************************************/

#ifndef CSU_CAN_H
#define CSU_CAN_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"


/* ************************** [[   define   ]]  *********************************************************** */

/* --- CAN 채널 (DevCan 설정 기준) --- */
/* CANA: GPIO71(TX), GPIO70(RX), 1Mbps, Extended Frame */
#define CSU_CAN_BASE            CANA_BASE

/* --- 메시지 오브젝트 (DevCan.h의 TX_MSG_OBJ_NO1, RX_MSG_OBJ_NO1 사용) --- */
/* TX: TX_MSG_OBJ_NO1 (1), RX: RX_MSG_OBJ_NO1 (1)  → DevCan.h 참조 */

/* --- CAN ID (DevCan.h 기준) --- */
/* TX ID: CANA_TX_EXT_ID_TEST1   (0x15555555) */
/* RX ID: CANA_RX_EXT_ID_H1_REQUEST (0x18FF30AD) */

/* --- 데이터 길이 (DevCan.h의 MSG_DATA_LENGTH = 8) --- */
/* MSG_DATA_LENGTH 사용 */


/* ************************** [[   enum or struct   ]]  *************************************************** */

/**
 * @brief CAN 모듈 상태 열거형
 */
typedef enum {
    eCAN_STATE_UNINIT = 0,      // 초기화 전
    eCAN_STATE_READY,           // 정상 대기
    eCAN_STATE_TX_PENDING,      // 송신 중
    eCAN_STATE_RX_OK,           // 수신 완료
    eCAN_STATE_BUS_OFF,         // 버스 오프 오류
    eCAN_STATE_ERROR            // 오류 발생
} eCanState;

/**
 * @brief PC(uCAN)로부터 수신되는 CAN 메시지 구조체 (LED 제어용)
 */
typedef struct {
    uint16_t IncNumber;
    uint16_t LedData;        // Byte[1]: LED01~08 상태 (Bit 0~7)
} stRcvCanMsg;

/**
 * @brief MCU에서 PC(uCAN)로 송신하는 CAN 메시지 구조체 (상태 보고용)
 */
typedef struct {
    uint16_t IncNumber;      // 시퀀스 (0~255)
    uint16_t Status;         // Tact01, Tact02 상태 (Bit 0, 1)
} stXmtCanMsg;


/* ************************** [[   global   ]]  *********************************************************** */
extern stRcvCanMsg  xRcvCanMsg;
extern stXmtCanMsg  xXmtCanMsg;
extern eCanState    xCanState;

/* Rx 수신 성공 플래그 (메인 루프에서 클리어) */
extern bool         bCanRxFlag;


/* ************************** [[  function  ]]  *********************************************************** */

/**
 * @brief CAN 모듈 초기화
 *        내부적으로 DevCan의 InitialCan() 을 호출하며
 *        CSU 레벨 구조체만 초기화합니다.
 *        DevDspInit.c의 InitialPeripherals()에서 호출하세요.
 */
void Initial_CAN(void);

/**
 * @brief ISR에서 채워진 rxMsgData1[]를 파싱하여 xRcvCanMsg 구조체 업데이트
 *        bCanRxFlag가 true일 때 메인 루프(10ms)에서 호출합니다.
 */
void recvCanMessage(void);

/**
 * @brief xXmtCanMsg 구조체 데이터를 8바이트로 직렬화하여 CAN 송신 (10ms 주기)
 *        내부적으로 DevCan의 SendCanaMessage() 를 호출합니다.
 */
void sendCanMessage(void);

/**
 * @brief CAN 모듈 상태 모니터링 (10ms 주기)
 *        Bus-Off 감지 시 복구 시도
 */
void updateCanStatus(void);

/**
 * @brief 실시간 센서 데이터를 CAN 송신 구조체에 반영 (10ms 주기)
 */
void updateCanXmtData(void);

#endif  // #ifndef CSU_CAN_H
