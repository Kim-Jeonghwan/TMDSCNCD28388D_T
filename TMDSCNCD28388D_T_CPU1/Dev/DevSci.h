/**********************************************************************

	Nexcom Co., Ltd.
	Copyright 2021. All Rights Reserved.

	Filename		: DevSci.h
	Version			: 00.00
	Description		: 
	Tracebility		: 
	Programmer	    :
	Last Updated	: 2026. 05. 15.

**********************************************************************/

/*
 * Modification History
 * --------------------
 * 
 * 
*/


#ifndef DEVSCI_H
#define DEVSCI_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"


/* ************************** [[   define   ]]  *********************************************************** */
#define QUEUE_MAX_SCI 200u


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



#endif	// #ifndef DEVSCI_H
