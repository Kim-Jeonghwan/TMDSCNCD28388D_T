/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_LED.h
    Description      : System Status LED Control (Green / Orange)
    Last Updated     : 2026. 01. 30.
**********************************************************************/

#ifndef CSU_LED_H
#define CSU_LED_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"



/* ************************** [[   define   ]]  *********************************************************** */
#define LED_OFF		false
#define LED_ON		true

#define LED_NONE	false
#define LED_TOGGLE	true

/* POWERON 상태 표시용 LED(GPIO 31)*/
#define GPIO_LED_POWERON     31u

/* 배열 LED (GPIO 32~39) */
#define GPIO_LED_01          32u // 
#define GPIO_LED_02          33u // 
#define GPIO_LED_03          34u // 
#define GPIO_LED_04          35u // 
#define GPIO_LED_05          36u // 
#define GPIO_LED_06          37u // 
#define GPIO_LED_07          38u // 
#define GPIO_LED_08          39u //  

/* ************************** [[   enum or struct   ]]  **************************************************** */

/**
 * @brief LED 인덱스 정의 (GPIO 번호 매핑)
 */
typedef enum
{
	eLED_POWERON			    = 31u,
	eLED_01					    = 32u,
    eLED_02                     = 33u,
	eLED_03				    	= 34u,
    eLED_04                     = 35u,
    eLED_05                     = 36u,
    eLED_06				    	= 37u,
	eLED_07				    	= 38u,
	eLED_08				    	= 39u

}eLed;

/**
 * @brief 개별 LED 제어 속성 구조체
 */
typedef struct
{
    uint16_t Index:8u;    // GPIO Index (eLed 타입 저장)
    uint16_t Time:8u;     // Toggle 주기 설정
    uint16_t Temp:8u;     // 카운트 다운용 임시 변수
    bool     State:1u;    // 현재 점등 상태 (false: Off, true: On)
    bool     Toggle:1u;   // 토글 모드 활성 (false: None, true: Toggle)
    uint16_t Reserved:14u;
} stLed;

/**
 * @brief 시스템 전체 LED 상태 관리 구조체
 */
typedef struct
{
	stLed	ledPowerOn;
	stLed	led01;
    stLed   led02;
    stLed	led03;    
    stLed	led04;
    stLed   led05;
    stLed	led06;
    stLed	led07;
    stLed	led08;

}stLedStatus;



/* ************************** [[   global   ]]  *********************************************************** */
extern stLedStatus xLed;



/* ************************** [[  function  ]]  *********************************************************** */

/**
 * @brief LED 제어를 위한 GPIO 방향 및 Mux 설정
 */
void initGpioDoutLed(void);

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
void setLedStatus(stLed *pLed, bool State);

/**
 * @brief LED 토글 모드 활성화 및 주기 설정
 */
void setLedModeToggle(stLed *pLed, bool State, uint16_t Time);

/**
 * @brief IPC 커맨드에 따른 GPIO LED 직접 제어
 */
void updateGpioLed(void);



#endif	// #ifndef CSU_LED_H

