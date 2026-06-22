/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Sci.h
    Version          : 00.00
    Description      : CPU1 SCI(UART) 통신 드라이버 헤더
    Programmer       : Kim Jeonghwan
 	Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비 및 매크로 이동)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (작성자 기입, 매크로 상수 이동 및 이력 보완)
 */


#ifndef HAL_SCI_H
#define HAL_SCI_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main_cpu1.h"


/* ************************** [[   define   ]]  *********************************************************** */
#define QUEUE_MAX_SCI 200u

#define SCI_PC_GPIO_PIN_SCIA_RXD	28u             // SCI RX용 GPIO 핀 번호
#define SCI_PC_GPIO_PIN_SCIA_TXD	29u             // SCI TX용 GPIO 핀 번호
#define SCI_PC_GPIO_CFG_SCIA_RXD	GPIO_28_SCIA_RX	// SCI RX용 pinConfig
#define SCI_PC_GPIO_CFG_SCIA_TXD	GPIO_29_SCIA_TX	// SCI TX용 pinConfig


/* ************************** [[   enum or struct   ]]  *************************************************** */
typedef enum
{
	eSciA_SOF = 0,
	eSciA_MSGID,
	eSciA_LEN,
	eSciA_DATA,
	eSciA_CRC,
	eSciA_EOT
}eSciA;

typedef struct
{
    eSciA           Frame;      /* 프레임 수신 위치 (SOF, ID, DATA 등 상태 제어) */
    uint16_t          MSGID;      /* 메시지 식별 ID (예: 0x10) */
    uint16_t          LEN;        /* 메시지 데이터 길이 (Payload Length) */
    uint16_t          DATA[50u];  /* 실제 수신 데이터 버퍼 */
    uint16_t          CRC;        /* 수신된 체크섬/CRC 값 */
    uint16_t 			POS;        /* 현재 데이터 수신/파싱 인덱스 위치 */
} stSciA;



typedef struct
{
    uint16_t front;
    uint16_t rear;
    uint16_t Data[QUEUE_MAX_SCI];
} stQsci;






/* ************************** [[   global   ]]  *********************************************************** */


/* ************************** [[  function  ]]  *********************************************************** */
void Initial_SCI(void);



__interrupt void isrScia_SCI_PC(void);

void xmtScia_SCI_PC(uint16_t data[], uint16_t len);

void sendScia_SCI_PC(void);



#endif	// #ifndef HAL_SCI_H
