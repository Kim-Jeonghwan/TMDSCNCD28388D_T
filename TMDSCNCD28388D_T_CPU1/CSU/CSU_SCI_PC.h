/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_SCI_PC.h
    Description      : PC Interface Communication (SCI_PC) Protocol Definition
    Last Updated     : 2026. 04. 30. (루프백 결과 비트 추가)
**********************************************************************/

#ifndef CSU_SCI_PC_H
#define CSU_SCI_PC_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"


/* ************************** [[   define   ]]  *********************************************************** */
/* 통신 패킷 관련 상수는 CSU_SCI_PC.c에 정의됨 (SOF: 0x7E, EOT: 0x0D, ID: 0x10) */


/* ************************** [[   enum or struct   ]]  *************************************************** */



/**
 * @brief PC 제어 명령 비트필드 공용체 (ID: 0x10)
 */
typedef union {
    uint16_t all;
    struct {
        bool LED01:1u;     // Bit 0: LED01 Enable
        bool LED02:1u;     // Bit 1: LED02 Enable
        bool LED03:1u;     // Bit 2: LED03 Enable
        bool LED04:1u;     // Bit 3: LED04 Enable
        bool LED05:1u;     // Bit 4: LED05 Enable
        bool LED06:1u;     // Bit 5: LED06 Enable
        bool LED07:1u;     // Bit 6: LED07 Enable
        bool LED08:1u;     // Bit 7: LED08 Enable
        bool EepWrite:1u;     // Bit 8: EEPROM Write Enable
        bool EepRead:1u;     // Bit 9: EEPROM Read Enable
        bool Epwm7aEn:1u;     // Bit 10: Epwm7a Enable
        bool LoopbackTest:1u; // Bit 11: Ethernet Loopback Test
        uint16_t Reserved:4u; // Bit 12-15: Reserved
    } bit;
} uSciPcCmd;

/**
 * @brief PC로부터 수신되는 메시지 구조체 (ID: 0x10)
 */
typedef struct {
    uint16_t IncNumber;
    uSciPcCmd  Command;         // Command(2byte)
    uint16_t EepAddr;        // EEPROM 주소 (2 bytes)
    uint16_t EepromWriteVal;  // EEPROM 데이터 (1 byte)
    uint16_t Epwm7aDuty;     // Epwm7a Duty (1 byte)
    uint16_t Epwm7aFreq;     // Epwm7a Freq (1 byte)
} stRcvSciPcMsg1;


/**
 * @brief MCU에서 PC로 송신하는 상태 보고 구조체 (ID: 0x10)
 * @details 총 데이터 영역 길이: 17 Bytes
 * 구조: IncNumber(1) + Data(16)
 */
typedef struct
{
    /* --- 1. Sequence Number (1 byte) --- */
    uint16_t          IncNumber:8u;   // Sequence Number (0~255)
    
    /* --- 2. 상태 및 에러 플래그 (Status - 1 byte) --- */
    bool              Tact01:1u;     // Bit 0: 스위치 1
    bool              Tact02:1u;      // Bit 1: 스위치 2
    bool              EthLoop1:1u;    // Bit 2: 이더넷 루프백 결과 1
    bool              EthLoop2:1u;    // Bit 3: 이더넷 루프백 결과 2
    uint16_t          Reserved:4u; // Bit 4-7: Reserved

    /* --- 3. 데이터 필드 (14 bytes) --- */
    uint16_t          EncoderAngle;   // Buf[2~3]
    uint16_t          EncoderRawPD:8u;   // Buf[4]
    uint16_t          EepromReadVal:8u;   // Buf[5]
    uint16_t          PWMRaw;   // Buf[6~7]
    uint16_t          PWMRCLPF;   // Buf[8~9]
    uint16_t          PWMBWLPF;   // Buf[10~11]
    uint16_t          PotenRAW;   // Buf[12~13]
    uint16_t          PotenMAVE;   // Buf[14~15]
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

/**
 * @brief 루프백 테스트 명령 업데이트
 */
void updateLoopbackTest(void);

#endif	// #ifndef CSU_SCI_PC_H

