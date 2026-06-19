/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_SCI_PC.c
    Description      : PC Interface Communication (SCI_PC) Protocol Definition
    Last Updated     : 2026. 06. 05. (코드 주석 포맷팅 및 한글화)
***********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "csu_SCI_PC.h"

/* 외부 참조 (xAdc 사용) */

/* ************************** [[   define   ]]  *********************************************************** */
#define SCI_PC_SOF		0x7Eu
#define SCI_PC_EOT		0x0Du
#define SCI_PC_MSG1	    0x10u

/* ************************** [[   global   ]]  *********************************************************** */
stRcvSciPcMsg1	xRcvSciPcMsg1;
stXmtSciPcMsg1	xXmtSciPcMsg1;

/* ************************** [[  static prototype  ]]  *************************************************** */

/* ************************** [[  function  ]]  *********************************************************** */

/*
@funtion    void recvSciPcMessage(uint16_t ID, uint16_t Data[])
@brief      PC로부터 수신된 SCI_PC 메시지 해석 및 구조체 업데이트
@param      ID: 수신된 메시지의 식별 번호 (0x10u)
@param      Data: 수신된 데이터 배열 (바이트 단위)
@return     void
@remark
    - ID 0x10 패킷을 수신하여 시퀀스 번호 및 제어 명령(Command)을 파싱합니다.
*/
void recvSciPcMessage(uint16_t ID, uint16_t Data[])
{
    volatile uint16_t pos = 0u;
    onConv16 on16;
    
    switch(ID)
    {
    case 0x10u:
        // on16 공용체를 활용하여 가독성 높고 안전하게 수신 데이터를 16비트로 조립 대입!
        on16.byte.B0 = (uint8_t)(Data[pos++] & 0xFFu); // Sequence Number
        on16.byte.B1 = (uint8_t)(Data[pos++] & 0xFFu); // 제어 명령 (Command)
        xRcvSciPcMsg1.all = on16.u16;
        break;

    default:
        break;
    }
}

/*
@funtion    void sendSciPcMessage1(void)
@brief      엔코더 상태 및 데이터를 PC로 전송 (10ms 주기)
@param      void
@return     void
@remark
    - 전체 9바이트 패킷을 구성하며, Length 필드와 실효 데이터 영역의 총합은 5바이트입니다.
    - 패킷 구조: SOF(1) + ID(1) + LEN(1) + DATA(4) + Checksum(1) + EOT(1)
*/
void sendSciPcMessage1(void)
{
    volatile uint16_t pos = 0u;
    onConv16 on16;
    uint16_t i = 0u;
    uint16_t Buf[15u] = {0u, }; 
    uint16_t CheckSum = 0u;

    /* 1. 헤더 구성 */
    Buf[pos++] = SCI_PC_SOF;           // Buf[0]: 0x7E (SOF)
    Buf[pos++] = SCI_PC_MSG1;          // Buf[1]: 0x10 (Msg ID)
    Buf[pos++] = 0u;                   // Buf[2]: Length (계산 전 임시 기입)
    
    /* 2. Sequence Number */
    Buf[pos++] = (uint16_t)(xXmtSciPcMsg1.IncNumber++ & 0xFFu); // Buf[3]

    /* 3. Status (1 byte) */
    Buf[pos++] = (uint16_t)(xXmtSciPcMsg1.Status & 0xFFu); // Buf[4]

    /* 4. DspTemp (uint16_t - 2 bytes, Little Endian, x10 스케일 및 반올림 적용) */
    /* float 온도(℃)를 x10 스케일 16bit 정수로 변환 후 전송 (반올림 포함) */
    xXmtSciPcMsg1.DspTemp = (uint16_t)((xAdc.currentTemperatureC * 10.0f) + 0.5f);
    on16.u16 = xXmtSciPcMsg1.DspTemp;
    Buf[pos++] = on16.byte.B0; // Low Byte (Buf[5])
    Buf[pos++] = on16.byte.B1; // High Byte (Buf[6])

    /* 5. 데이터 길이(LEN) 계산 및 업데이트 */
    Buf[2] = (uint16_t)(pos - 2u);

    /* 6. CheckSum 계산 (Length인 Buf[2]부터 데이터 끝인 Buf[pos-1]까지 합산) */
    CheckSum = 0u;
    for(i = 2u; i < pos; i++)
    {
        CheckSum += (Buf[i] & 0xFFu);
    }
    Buf[pos++] = (uint16_t)(CheckSum & 0xFFu); // Buf[7]: Checksum
    Buf[pos++] = SCI_PC_EOT;                   // Buf[8]: 0x0D (EOT)

    /* 6. 최종 전송 (총 9바이트) */
    xmtScia_SCI_PC(Buf, pos);
}
