/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_LED.c
    Description      : System Status LED Control (Green / Orange)
    Last Updated     : 2026. 06. 01. (LEDError 추가 및 빌드 경고/오류 핫픽스)
**********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "CSU_LED.h"


/* ************************** [[   define   ]]  *********************************************************** */



/* ************************** [[   define   ]]  *********************************************************** */
// 내부 온도 센서(ADCC13) 에러 임계 온도 기준 (나중에 여기 값을 변경하여 적용 가능)
#define LIMIT_TEMP_ERROR       45.0f  


/* ************************** [[   global   ]]  *********************************************************** */
stLedStatus xLed;

// CSU_Adc.c에 선언된 디버깅용 실시간 온도 전역 변수
extern float32_t currentTemperatureC;


/* ************************** [[  static prototype  ]]  *************************************************** */
static void HW_writeLedPin(uint16_t Index, bool State); 
static void HW_toggleLedPin(uint16_t Index);


/* ************************** [[  function  ]]  *********************************************************** */

/**
 * @brief LED 관련 GPIO 초기화
 */
void initGpioDoutLed(void)
{
    EALLOW;
    
    // RUN LED (GPIO145)
    GPIO_setPinConfig(GPIO_145_GPIO145);
    GPIO_setPadConfig(145, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(145, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(145, GPIO_CORE_CPU1);

    // ERROR LED (GPIO146)
    GPIO_setPinConfig(GPIO_146_GPIO146);
    GPIO_setPadConfig(146, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(146, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(146, GPIO_CORE_CPU1);

    // LED 01 (GPIO31)
    GPIO_setPinConfig(GPIO_31_GPIO31);
    GPIO_setPadConfig(31, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(31, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(31, GPIO_CORE_CPU1);
    
    // LED 02 (GPIO32)
    GPIO_setPinConfig(GPIO_32_GPIO32);
    GPIO_setPadConfig(32, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(32, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(32, GPIO_CORE_CPU1);

    // LED 03 (GPIO33)
    GPIO_setPinConfig(GPIO_33_GPIO33);
    GPIO_setPadConfig(33, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(33, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(33, GPIO_CORE_CPU1);

    // LED 04 (GPIO34)
    GPIO_setPinConfig(GPIO_34_GPIO34);
    GPIO_setPadConfig(34, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(34, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(34, GPIO_CORE_CPU1);

    // LED 05 (GPIO35)
    GPIO_setPinConfig(GPIO_35_GPIO35);
    GPIO_setPadConfig(35, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(35, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(35, GPIO_CORE_CPU1);

    // LED 06 (GPIO36)
    GPIO_setPinConfig(GPIO_36_GPIO36);
    GPIO_setPadConfig(36, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(36, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(36, GPIO_CORE_CPU1);

    // LED 07 (GPIO37)
    GPIO_setPinConfig(GPIO_37_GPIO37);
    GPIO_setPadConfig(37, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(37, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(37, GPIO_CORE_CPU1);

    // LED 08 (GPIO38)
    GPIO_setPinConfig(GPIO_38_GPIO38);
    GPIO_setPadConfig(38, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(38, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(38, GPIO_CORE_CPU1);
    
    EDIS;
}

/**
 * @brief LED 초기값 설정
 * @details Green LED는 시스템 정상 동작 확인용으로 1초 토글 설정을 기본으로 합니다.
 */
void Initial_LED(void)
{
    // RUN LED (GPIO145) 설정
    xLed.ledRun.Index  = eLED_RUN;
    setLedModeToggle(&xLed.ledRun, LED_TOGGLE, 10u); // 1초 주기 토글

    // ERROR LED (GPIO146) 설정
    xLed.ledError.Index = eLED_ERROR;
    setLedModeToggle(&xLed.ledError, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.ledError, LED_OFF);

    // 01 LED (GPIO 31) 설정
    xLed.led01.Index = eLED_01;
    setLedModeToggle(&xLed.led01, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led01, LED_OFF);

    // 02 LED (GPIO 32) 설정
    xLed.led02.Index = eLED_02;
    setLedModeToggle(&xLed.led02, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led02, LED_OFF);
    
    // 03 LED (GPIO 33) 설정
    xLed.led03.Index = eLED_03;
    setLedModeToggle(&xLed.led03, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led03, LED_OFF);
    
    // 04 LED (GPIO 34) 설정
    xLed.led04.Index = eLED_04;
    setLedModeToggle(&xLed.led04, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led04, LED_OFF);

    // 05 LED (GPIO 35) 설정
    xLed.led05.Index = eLED_05;
    setLedModeToggle(&xLed.led05, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led05, LED_OFF);

    // 06 LED (GPIO 36) 설정
    xLed.led06.Index = eLED_06;
    setLedModeToggle(&xLed.led06, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led06, LED_OFF);

    // 07 LED (GPIO 37) 설정
    xLed.led07.Index = eLED_07;
    setLedModeToggle(&xLed.led07, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led07, LED_OFF);

    // 08 LED (GPIO 38) 설정
    xLed.led08.Index = eLED_08;
    setLedModeToggle(&xLed.led08, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led08, LED_OFF);


}

/**
 * @brief LED 상태 머신 업데이트
 * @details 100ms 주기로 호출되어 토글 카운트를 관리하거나 On/Off 상태를 유지합니다.
 */
void updateLedStatus(void)
{
    uint16_t i = 0u;
    // Run LED + Error LED + LED 01~08까지 총 10개 관리
    stLed *pLed[10];
    
    // 1. 구조체 포인터 배열 매핑
    pLed[0] = &xLed.ledRun;
    pLed[1] = &xLed.ledError;
    pLed[2] = &xLed.led01;
    pLed[3] = &xLed.led02;
    pLed[4] = &xLed.led03;
    pLed[5] = &xLed.led04;
    pLed[6] = &xLed.led05;
    pLed[7] = &xLed.led06;
    pLed[8] = &xLed.led07;
    pLed[9] = &xLed.led08;


    // 3. 내부 온도 센서(ADCC13) 실시간 감시를 통한 ERROR LED 제어
    if(currentTemperatureC >= LIMIT_TEMP_ERROR)
    {
        setLedStatus(&xLed.ledError, LED_ON);
    }
    else
    {
        setLedStatus(&xLed.ledError, LED_OFF);
    }

    // 4. 전체 LED 상태 업데이트 루프
    for(i = 0u; i < 10u; i++)
    {
        if(pLed[i]->Toggle == LED_TOGGLE)
        {
            if(pLed[i]->Temp == 0u)
            {
                HW_toggleLedPin(pLed[i]->Index);
                pLed[i]->Temp = pLed[i]->Time;
            }
            else
            {
                pLed[i]->Temp--;
            }
        }
        else
        {
            // State 값에 따라 물리 핀 출력
            HW_writeLedPin(pLed[i]->Index, pLed[i]->State);
        }
    }
}


/**
 * @brief LED 강제 점등/소등 설정
 * @param pLed 대상 LED 구조체 포인터
 * @param State LED_ON(1) 또는 LED_OFF(0)
 */
void setLedStatus(stLed *pLed, bool State)
{
    if(pLed != NULL)
    {
        if(pLed->State != State)
        {
            pLed->State = State;
            pLed->Toggle = LED_NONE; 
            HW_writeLedPin(pLed->Index, State);
        }
    }
}


/**
 * @brief LED 토글 동작 설정
 * @param State LED_TOGGLE(1) 또는 LED_NONE(0)
 * @param Time 토글 주기 카운트
 */
void setLedModeToggle(stLed *pLed, bool State, uint16_t Time)
{
    if(pLed != NULL)
    {
        pLed->Toggle = State;
        pLed->Time   = Time;
        pLed->Temp   = 0u;
    }
}



/**
 * @brief 하드웨어 GPIO 핀 직접 제어 (eLed enum 및 Driverlib 적용)
 */
static void HW_writeLedPin(uint16_t Index, bool State)
{
	switch(Index)
	{
	case eLED_RUN:
		GPIO_writePin(eLED_RUN, (uint32_t)State);
		break;

	case eLED_ERROR:
		GPIO_writePin(eLED_ERROR, (uint32_t)State);
		break;

	case eLED_01:
		GPIO_writePin(eLED_01, (uint32_t)State);
		break;

	case eLED_02:
		GPIO_writePin(eLED_02, (uint32_t)State);
		break;

	case eLED_03:
		GPIO_writePin(eLED_03, (uint32_t)State);
		break;

	case eLED_04:
		GPIO_writePin(eLED_04, (uint32_t)State);
		break;

	case eLED_05:
		GPIO_writePin(eLED_05, (uint32_t)State);
		break;

	case eLED_06:
		GPIO_writePin(eLED_06, (uint32_t)State);
		break;

	case eLED_07:
		GPIO_writePin(eLED_07, (uint32_t)State);
		break;

	case eLED_08:
		GPIO_writePin(eLED_08, (uint32_t)State);
		break;

	default:
		// MISRA
		break;
	}
}

/**
 * @brief 하드웨어 GPIO 핀 토글 제어 (eLed enum 및 Driverlib 적용)
 */
static void HW_toggleLedPin(uint16_t Index)
{
	switch(Index)
	{
	case eLED_RUN:
		GPIO_togglePin(eLED_RUN);
		break;

	case eLED_ERROR:
		GPIO_togglePin(eLED_ERROR);
		break;

	case eLED_01:
		GPIO_togglePin(eLED_01);
		break;

	case eLED_02:
		GPIO_togglePin(eLED_02);
		break;

	case eLED_03:
		GPIO_togglePin(eLED_03);
		break;

	case eLED_04:
		GPIO_togglePin(eLED_04);
		break;

	case eLED_05:
		GPIO_togglePin(eLED_05);
		break;

	case eLED_06:
		GPIO_togglePin(eLED_06);
		break;

	case eLED_07:
		GPIO_togglePin(eLED_07);
		break;

	case eLED_08:
		GPIO_togglePin(eLED_08);
		break;

	default:
		// MISRA
		break;
	}
}
