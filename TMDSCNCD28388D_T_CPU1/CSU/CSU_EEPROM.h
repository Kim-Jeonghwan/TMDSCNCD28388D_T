/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_EEPROM.h
    Description      : EEPROM M95M04 SPI 통신 드라이버
    Last Updated     : 2026. 04. 13.
**********************************************************************/

#ifndef CSU_EEPROM_H
#define CSU_EEPROM_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"

/* ************************** [[   define   ]]  *********************************************************** */

/**
 * @brief EEPROM M95M04 명령어 셋 (Instruction Set)
 */
#define EEPROM_CMD_WREN     0x06u    // 쓰기 활성화 (Write Enable)
#define EEPROM_CMD_WRDI     0x04u    // 쓰기 비활성화 (Write Disable)
#define EEPROM_CMD_RDSR     0x05u    // 상태 레지스터 읽기 (Read Status Register)
#define EEPROM_CMD_WRSR     0x01u    // 상태 레지스터 쓰기 (Write Status Register)
#define EEPROM_CMD_READ     0x03u    // 메모리 배열에서 읽기 (Read from Memory Array)
#define EEPROM_CMD_WRITE    0x02u    // 메모리 배열에 쓰기 (Write to Memory Array)

/**
 * @brief EEPROM 칩 셀렉트 (CS) 매크로 (GPIO61 사용)
 */
#define EEPROM_CS_LOW()     GPIO_writePin(61, 0)
#define EEPROM_CS_HIGH()    GPIO_writePin(61, 1)

/* ************************** [[   enum or struct   ]]  *************************************************** */


/* ************************** [[   global   ]]  *********************************************************** */
extern uint16_t eeprom_read_val;

/* ************************** [[  function  ]]  *********************************************************** */

/**
 * @brief EEPROM 관련 초기화
 */
void EEPROM_Init(void);

/**
 * @brief EEPROM 쓰기 활성화 (WEL 비트 설정)
 */
void EEPROM_WriteEnable(void);

/**
 * @brief EEPROM 상태 레지스터 읽기
 * @return 8-bit 상태 값
 */
uint16_t EEPROM_ReadStatus(void);

/**
 * @brief EEPROM 1Byte 쓰기
 * @param address 24-bit 주소
 * @param data 저장할 1Byte 데이터
 */
void EEPROM_WriteByte(uint32_t address, uint16_t data);

/**
 * @brief EEPROM 1Byte 읽기
 * @param address 24-bit (실제 19-bit 사용) 주소 (0x000000 ~ 0x07FFFF)
 * @return 1Byte 데이터
 */
uint16_t EEPROM_ReadByte(uint32_t address);

/**
 * @brief EEPROM 동작 테스트 함수
 */
void EEPROM_Test(void);

/**
 * @brief IPC 커맨드에 따른 EEPROM 읽기/쓰기 스케줄러 처리
 */
void updateEepromStatus(void);


#endif // CSU_EEPROM_H

