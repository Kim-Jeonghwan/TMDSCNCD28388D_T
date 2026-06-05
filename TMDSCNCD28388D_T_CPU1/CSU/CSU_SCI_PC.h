/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_SCI_PC.h
    Description      : PC Interface Communication (SCI_PC) Protocol Definition
    Last Updated     : 2026. 06. 05. (코드 주석 포맷팅 및 한글화)
**********************************************************************/

#ifndef CSU_SCI_PC_H
#define CSU_SCI_PC_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"

/* ************************** [[   define   ]]  *********************************************************** */
/* 통신 패킷 관련 상수는 CSU_SCI_PC.c에 정의됨 (SOF: 0x7E, EOT: 0x0D, ID: 0x10) */

/* ************************** [[   enum or struct   ]]  *************************************************** */

/**
 * @brief PC로부터 수신되는 제어 명령 공용체 구조체 (ID: 0x10, 데이터 영역: 총 2 Bytes = 16비트 = 1 Word)
 * @details 하위 8비트는 IncNumber, 상위 8비트는 Reserved1~8 비트필드로 구성되어 단 1 Word 내에 완벽하게 안착합니다.
 */
typedef union {
    uint16_t all;
    struct {
        /* --- 하위 8비트: Sequence Number (Byte 3) --- */
        uint16_t          IncNumber:8u;   
        
        /* --- 상위 8비트: 제어 명령 비트필드 (Byte 4, Reserved1~8) --- */
        bool              Reserved1:1u;   // Bit 8: Reserved 1
        bool              Reserved2:1u;   // Bit 9: Reserved 2
        bool              Reserved3:1u;   // Bit 10: Reserved 3
        bool              Reserved4:1u;   // Bit 11: Reserved 4
        bool              Reserved5:1u;   // Bit 12: Reserved 5
        bool              Reserved6:1u;   // Bit 13: Reserved 6
        bool              Reserved7:1u;   // Bit 14: Reserved 7
        bool              Reserved8:1u;   // Bit 15: Reserved 8
    } bit;
} stRcvSciPcMsg1;

/**
 * @brief MCU에서 PC로 송신하는 상태 보고 구조체 (ID: 0x10, 데이터 영역: 4 Bytes)
 */
typedef struct
{
    /* --- 1. Sequence Number (1 byte) --- */
    uint16_t          IncNumber:8u;   // Byte 3: Sequence Number (0~255)
    
    /* --- 2. 상태 및 에러 플래그 (Status - 1 byte) --- */
    uint16_t          Status:8u;      // Byte 4: MCU 상태 필드 (개별 bit 제어)

    /* --- 3. 데이터 필드 (DspTemp - 2 bytes) --- */
    uint16_t          DspTemp;        // Byte 5~6: DSP 정션 온도 (소수점 한자리 표현용 ×10 스케일링 값)
} stXmtSciPcMsg1;



/* ************************** [[   global   ]]  *********************************************************** */
extern stRcvSciPcMsg1 xRcvSciPcMsg1;
extern stXmtSciPcMsg1	xXmtSciPcMsg1;



/* ************************** [[  function  ]]  *********************************************************** */
/**
 * @brief PC로부터 수신된 SCI_PC 메시지를 해석하여 구조체에 저장
 */
void recvSciPcMessage(uint16_t ID, uint16_t Data[]);

/**
 * @brief 현재 시스템 상태 및 엔코더 데이터를 PC로 송신 (10ms 주기)
 */
void sendSciPcMessage1(void);



#endif	// #ifndef CSU_SCI_PC_H
