/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_LED.c
    Description      : System Status LED Control (Green / Orange)
    Last Updated     : 2026. 05. 29. (정적 시험 기준 준수 및 보안 취약점 보완)
**********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "CSU_LED.h"


/* ************************** [[   define   ]]  *********************************************************** */



/* ************************** [[   global   ]]  *********************************************************** */
stLedStatus xLed;


/* ************************** [[  static prototype  ]]  *************************************************** */
static void HW_writeLedPin(uint16_t Index, bool State); 
static void HW_toggleLedPin(uint16_t Index);


/* ************************** [[  function  ]]  *********************************************************** */

/**
 * @brief LED 관련 GPIO 초기화
 */
void initGpioDoutLed(void)
{
	// LED POWERON (GPIO31)
	GPIO_SetupPinMux(eLED_POWERON, GPIO_MUX_CPU1, 0u);
	GPIO_SetupPinOptions(eLED_POWERON, GPIO_OUTPUT, GPIO_PUSHPULL);

	// LED 01 (GPIO32)
	GPIO_SetupPinMux(eLED_01, GPIO_MUX_CPU1, 0u);
	GPIO_SetupPinOptions(eLED_01, GPIO_OUTPUT, GPIO_PUSHPULL);

    // LED 02 (GPIO33)
	GPIO_SetupPinMux(eLED_02, GPIO_MUX_CPU1, 0u);
	GPIO_SetupPinOptions(eLED_02, GPIO_OUTPUT, GPIO_PUSHPULL);
    
	// LED 03 (GPIO34)
	GPIO_SetupPinMux(eLED_03, GPIO_MUX_CPU1, 0u);
	GPIO_SetupPinOptions(eLED_03, GPIO_OUTPUT, GPIO_PUSHPULL);

    // LED 04 (GPIO35)
	GPIO_SetupPinMux(eLED_04, GPIO_MUX_CPU1, 0u);
	GPIO_SetupPinOptions(eLED_04, GPIO_OUTPUT, GPIO_PUSHPULL);
    
    // LED 05 (GPIO36)
	GPIO_SetupPinMux(eLED_05, GPIO_MUX_CPU1, 0u);
	GPIO_SetupPinOptions(eLED_05, GPIO_OUTPUT, GPIO_PUSHPULL);

    // LED 06 (GPIO37)
	GPIO_SetupPinMux(eLED_06, GPIO_MUX_CPU1, 0u);
	GPIO_SetupPinOptions(eLED_06, GPIO_OUTPUT, GPIO_PUSHPULL);

    // LED 07 (GPIO38)
	GPIO_SetupPinMux(eLED_07, GPIO_MUX_CPU1, 0u);
	GPIO_SetupPinOptions(eLED_07, GPIO_OUTPUT, GPIO_PUSHPULL);

    // LED 08 (GPIO39)
	GPIO_SetupPinMux(eLED_08, GPIO_MUX_CPU1, 0u);
	GPIO_SetupPinOptions(eLED_08, GPIO_OUTPUT, GPIO_PUSHPULL);


}

/**
 * @brief LED 초기값 설정
 * @details Green LED는 시스템 정상 동작 확인용으로 1초 토글 설정을 기본으로 합니다.
 */
void Initial_LED(void)
{
    // POWERON LED (GPIO 31) 설정
    xLed.ledPowerOn.Index  = eLED_POWERON;
    setLedModeToggle(&xLed.ledPowerOn, LED_TOGGLE, 10u); // 1초 주기 토글

    // 01 LED (GPIO 32) 설정
    xLed.led01.Index = eLED_01;
    setLedModeToggle(&xLed.led01, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led01, LED_OFF);

    // 02 LED (GPIO 33) 설정
    xLed.led02.Index = eLED_02;
    setLedModeToggle(&xLed.led02, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led02, LED_OFF);

    // 03 LED (GPIO 34) 설정
    xLed.led03.Index = eLED_03;
    setLedModeToggle(&xLed.led03, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led03, LED_OFF);
    
    // 04 LED (GPIO 35) 설정
    xLed.led04.Index = eLED_04;
    setLedModeToggle(&xLed.led04, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led04, LED_OFF);
    
    // 05 LED (GPIO 36) 설정
    xLed.led05.Index = eLED_05;
    setLedModeToggle(&xLed.led05, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led05, LED_OFF);

    // 06 LED (GPIO 37) 설정
    xLed.led06.Index = eLED_06;
    setLedModeToggle(&xLed.led06, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led06, LED_OFF);

    // 07 LED (GPIO 38) 설정
    xLed.led07.Index = eLED_07;
    setLedModeToggle(&xLed.led07, LED_NONE, 0u);   // 기본 꺼짐
    setLedStatus(&xLed.led07, LED_OFF);

    // 08 LED (GPIO 39) 설정
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
    // PowerOn LED + LED 01~08까지 총 9개 관리
    stLed *pLed[9];
    
    // 1. 구조체 포인터 배열 매핑
    pLed[0] = &xLed.ledPowerOn;
    pLed[1] = &xLed.led01;
    pLed[2] = &xLed.led02;
    pLed[3] = &xLed.led03;
    pLed[4] = &xLed.led04;
    pLed[5] = &xLed.led05;
    pLed[6] = &xLed.led06;
    pLed[7] = &xLed.led07;
    pLed[8] = &xLed.led08;

    // 2. SCI_PC 메시지 데이터를 구조체 상태값으로 동기화 (기존 updateGpioLed 기능 통합)
    // 토글 모드가 아닐 때만 SCI_PC 메시지 값을 반영하도록 설계하는 것이 안전합니다.
    if(xRcvSciPcMsg1.Command.bit.LoopbackTest == 0)
    {
        xLed.led01.State = (bool)xRcvSciPcMsg1.Command.bit.LED01;
        xLed.led02.State = (bool)xRcvSciPcMsg1.Command.bit.LED02;
    }
    xLed.led03.State = (bool)xRcvSciPcMsg1.Command.bit.LED03;
    xLed.led04.State = (bool)xRcvSciPcMsg1.Command.bit.LED04;
    xLed.led05.State = (bool)xRcvSciPcMsg1.Command.bit.LED05;
    xLed.led06.State = (bool)xRcvSciPcMsg1.Command.bit.LED06;
    xLed.led07.State = (bool)xRcvSciPcMsg1.Command.bit.LED07;
    xLed.led08.State = (bool)xRcvSciPcMsg1.Command.bit.LED08;

    // 3. 전체 LED 상태 업데이트 루프
    for(i = 0u; i < 9u; i++)
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
	case eLED_POWERON:
		GPIO_writePin(eLED_POWERON, (uint32_t)State);
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
	case eLED_POWERON:
		GPIO_togglePin(eLED_POWERON);
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
