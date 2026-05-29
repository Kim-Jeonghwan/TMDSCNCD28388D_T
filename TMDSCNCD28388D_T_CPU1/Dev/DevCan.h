/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevCan.h
    Description      : CAN Device Driver Header (CANA, GPIO70/71, 1Mbps, Extended Frame, ISR)
    Last Updated     : 2026. 04. 17.
**********************************************************************/

#ifndef DEVCAN_H
#define DEVCAN_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"


/* ************************** [[   define   ]]  *********************************************************** */
#define MSG_DATA_LENGTH         8       // 테스트용 송수신 메시지 길이 (8-Bytes)

#define TX_MSG_OBJ_NO1          1       // CAN-A 모듈의 32개 메시지 오브젝트들 중 1번 MSG_OBJ를 TX용 MSG_OBJ 1번으로 사용
#define RX_MSG_OBJ_NO1          2       // CAN-A 모듈의 32개 메시지 오브젝트들 중 2번 MSG_OBJ를 RX용 MSG_OBJ 1번으로 사용

#define CANA_TX_EXT_ID_TEST1            0x15555555
//#define CANA_TX_EXT_ID_TEST3          0x801A05A4

#define CANA_RX_EXT_ID_H1_REQUEST       0x18FF30AD  // H1으로 TRIP 정보 요청 메시지
//#define CANA_RX_EXT_ID_TEST2          0x801A05A3
//#define CANA_RX_EXT_ID_TEST3          0x801A05A4


/* ************************** [[   global   ]]  *********************************************************** */
extern Uint16   errorFlag;
extern Uint16   rxMsgData1[MSG_DATA_LENGTH];    // ISR 수신 버퍼 (CanaISR에서 채움)


/* ************************** [[  function  ]]  *********************************************************** */
void InitialCan(void);
void InitialCana(void);

__interrupt void CanaISR(void);

void SendCanaMessage(Uint16 aMsgOBJ, Uint16 aLen, Uint16 aTxData[]);

#endif  // #ifndef DEVCAN_H
