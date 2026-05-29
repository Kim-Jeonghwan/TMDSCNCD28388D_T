/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_EEPROM.c
    Description      : EEPROM M95M04 SPI 통신 드라이버
    Last Updated     : 2026. 04. 17.
**********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "CSU_EEPROM.h"


/* ************************** [[   define   ]]  *********************************************************** */


/* ************************** [[   global   ]]  *********************************************************** */
uint16_t eeprom_read_val = 0;

/* ************************** [[  static prototype  ]]  *************************************************** */
static uint16_t SPIA_Transfer8(uint16_t byte_data);


/* ************************** [[  function  ]]  *********************************************************** */

/**
 * @brief EEPROM 관련 초기화
 */
void EEPROM_Init(void)
{
    // EEPROM 칩 셀렉트 핀은 DevSpi.c의 InitSpia() 에서 초기화됨 (High 대기)
    EEPROM_CS_HIGH();
}

/**
 * @brief SPIA 8-bit 송수신 함수
 * @param byte_data 송신할 8비트 데이터
 * @return 수신된 8비트 데이터
 */
static uint16_t SPIA_Transfer8(uint16_t byte_data)
{
    // TX 데이터는 좌측 정렬(Left Aligned)
    SPI_writeDataBlockingNonFIFO(SPIA_BASE, byte_data << 8);
    // RX 데이터는 우측 정렬(Right Aligned)
    return SPI_readDataBlockingNonFIFO(SPIA_BASE) & 0xFFu;
}

/**
 * @brief EEPROM 쓰기 활성화 (WEL 비트 설정)
 */
void EEPROM_WriteEnable(void)
{
    EEPROM_CS_LOW();
    (void)SPIA_Transfer8(EEPROM_CMD_WREN);
    EEPROM_CS_HIGH();
}

/**
 * @brief EEPROM 상태 레지스터 읽기
 * @return 8-bit 상태 값
 */
uint16_t EEPROM_ReadStatus(void)
{
    uint16_t status = 0;

    EEPROM_CS_LOW();
    (void)SPIA_Transfer8(EEPROM_CMD_RDSR);
    status = SPIA_Transfer8(0x00u);
    EEPROM_CS_HIGH();

    return status;
}

/**
 * @brief EEPROM 1Byte 읽기
 * @param address 24-bit (실제 19-bit 사용) 주소 (0x000000 ~ 0x07FFFF)
 * @return 1Byte 데이터
 */
uint16_t EEPROM_ReadByte(uint32_t address)
{
    uint16_t rxData = 0;

    EEPROM_CS_LOW();
    
    // 읽기 명령어 전송
    (void)SPIA_Transfer8(EEPROM_CMD_READ);
    
    // 24-bit 주소 전송
    (void)SPIA_Transfer8((uint16_t)((address >> 16) & 0xFFu)); // A23-A16
    (void)SPIA_Transfer8((uint16_t)((address >> 8) & 0xFFu));  // A15-A8
    (void)SPIA_Transfer8((uint16_t)(address & 0xFFu));         // A7-A0
    
    // 데이터 수신
    rxData = SPIA_Transfer8(0x00u);
    
    EEPROM_CS_HIGH();

    return rxData;
}

/**
 * @brief EEPROM 1Byte 쓰기
 * @param address 24-bit 주소
 * @param data 저장할 1Byte 데이터
 */
void EEPROM_WriteByte(uint32_t address, uint16_t data)
{
    // 쓰기 활성화
    EEPROM_WriteEnable();

    EEPROM_CS_LOW();
    
    // 쓰기 명령어 전송
    (void)SPIA_Transfer8(EEPROM_CMD_WRITE);
    
    // 24-bit 주소 전송
    (void)SPIA_Transfer8((uint16_t)((address >> 16) & 0xFFu)); // A23-A16
    (void)SPIA_Transfer8((uint16_t)((address >> 8) & 0xFFu));  // A15-A8
    (void)SPIA_Transfer8((uint16_t)(address & 0xFFu));         // A7-A0
    
    // 데이터 전송
    (void)SPIA_Transfer8(data & 0xFFu);
    
    EEPROM_CS_HIGH();
}

/**
 * @brief EEPROM 동작 테스트 함수
 */
void EEPROM_Test(void)
{
    uint32_t test_addr = 0x000100u;
    uint16_t write_val = 0xA5u;
    uint16_t read_val = 0;

    EEPROM_Init();
    EEPROM_WriteByte(test_addr, write_val);
    read_val = EEPROM_ReadByte(test_addr);

    if(read_val == write_val) {
        // 성공
    }
}

/**
 * @brief SCI_PC 커맨드에 따른 EEPROM 읽기/쓰기 처리 태스크
 */
void updateEepromStatus(void)
{
    // 1. EEPROM 쓰기 처리
    if (xRcvSciPcMsg1.Command.bit.EepWrite == true)
    {
        // 2바이트 주소와 1바이트 데이터를 사용하여 쓰기 수행
        EEPROM_WriteByte((uint32_t)xRcvSciPcMsg1.EepAddr, xRcvSciPcMsg1.EepromWriteVal);
        
        // 명령 처리 완료 후 비트 클리어
        xRcvSciPcMsg1.Command.bit.EepWrite = false;
    }

    // 2. EEPROM 읽기 처리
    if (xRcvSciPcMsg1.Command.bit.EepRead == true)
    {
        // 지정된 주소에서 데이터를 읽어 송신 메시지 구조체에 저장
        xXmtSciPcMsg1.EepromReadVal = EEPROM_ReadByte((uint32_t)xRcvSciPcMsg1.EepAddr);
        
        // 명령 처리 완료 후 비트 클리어
        xRcvSciPcMsg1.Command.bit.EepRead = false;
    }
}


