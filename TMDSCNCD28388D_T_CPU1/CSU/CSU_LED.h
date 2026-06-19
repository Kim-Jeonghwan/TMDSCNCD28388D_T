/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_LED.h
    Version          : 00.00
    Description      : System Status LED Control (Green / Orange)
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (ATTLA_T 방식의 LED 제어 로직 적용 및 최적화)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - 비트필드 구조체 제거, 자료형 통일, 사용 안하는 LED 핀 삭제 등 ATTLA_T 구조 동기화
 * 2026. 06. 05. - (코드 주석 포맷팅 및 한글화)
 */

#ifndef csu_LED_H
#define csu_LED_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"



/* ************************** [[   define   ]]  *********************************************************** */
#define LED_OFF		1u
#define LED_ON		0u

#define LED_NONE	0u
#define LED_TOGGLE	1u

/* RUN 상태 표시용 LED(GPIO 31)*/
#define GPIO_LED_RUN         31u

/* ************************** [[   enum or struct   ]]  **************************************************** */

/**
 * @brief LED 인덱스 정의 (GPIO 번호 매핑)
 */
typedef enum
{
	eLED_RUN			        = 31u,

}eLed;

/**
 * @brief 개별 LED 제어 속성 구조체
 */
typedef struct
{
    uint16_t Index;       // GPIO Index (eLed 타입 저장)
    uint16_t Time;        // Toggle 주기 설정
    uint16_t Temp;        // 카운트 다운용 임시 변수
    uint16_t State;       // 현재 점등 상태 (LED_ON: 0, LED_OFF: 1)
    uint16_t Toggle;      // 토글 모드 활성 (LED_NONE: 0, LED_TOGGLE: 1)
} stLed;

/**
 * @brief 시스템 전체 LED 상태 관리 구조체
 */
typedef struct
{
	stLed	ledRun;

}stLedStatus;



/* ************************** [[   global   ]]  *********************************************************** */
extern stLedStatus xLed;



/* ************************** [[  function  ]]  *********************************************************** */

/**
 * @brief LED 변수 초기화 및 기본 동작 설정
 */
void Initial_LED(void);

/**
 * @brief LED 동작 상태 업데이트 (Main Loop 호출)
 */
void updateLedStatus(void);

/**
 * @brief LED의 On/Off 상태를 직접 설정 (토글 중단)

 */
void setLedStatus(stLed *pLed, uint16_t State);

/**
 * @brief LED 토글 모드 활성화 및 주기 설정
 */
void setLedModeToggle(stLed *pLed, uint16_t State, uint16_t Time);

/**
 * @brief IPC 커맨드에 따른 GPIO LED 직접 제어
 */
void updateGpioLed(void);



#endif	// #ifndef csu_LED_H

