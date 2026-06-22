/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Led.c
    Version          : 00.02
    Description      : System Status LED Control (Green / Orange)
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비 및 매크로 이동)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (매크로 상수 헤더 이동 및 이력 보완)
 * 2026. 06. 22. - 물리 파일명을 csu_Led.c로 소문자 정정
 * 2026. 06. 19. - 불필요한 HAL 래퍼 함수 제거, GPIO 초기화 HAL 이관, ATTLA_T 구조 동기화
 * 2026. 06. 05. - (코드 주석 포맷팅 및 한글화)
 */

/* ************************** [[   include  ]]  *********************************************************** */
#include "csu_Led.h"


/* ************************** [[   define   ]]  *********************************************************** */



/* ************************** [[   global   ]]  *********************************************************** */
stLedStatus xLed;



/*
@funtion    void Initial_LED(void)
@brief      LED 모드 초기값 설정
@param      void
@return     void
@remark
    - RUN LED는 1초 토글.
*/
void Initial_LED(void)
{
    // RUN LED (GPIO31) 설정
    xLed.ledRun.Index  = eLED_RUN;
    setLedModeToggle(&xLed.ledRun, LED_TOGGLE, 4u); // 1초 주기 토글
}

/*
@funtion    void updateLedStatus(void)
@brief      LED 상태 머신 업데이트
@param      void
@return     void
@remark
    - 100ms 주기로 호출되며 각 LED의 토글 카운트를 갱신하고 물리 핀 출력을 제어합니다.
*/
void updateLedStatus(void)
{
    uint16_t i = 0u;
    // Run LED 1개 관리
    stLed *pLed[1];
    
    // 1. 구조체 포인터 배열 매핑
    pLed[0] = &xLed.ledRun;

    // 2. 전체 LED 상태 업데이트 루프
    for(i = 0u; i < 1u; i++)
    {
        if(pLed[i]->Toggle == LED_TOGGLE)
        {
            if(pLed[i]->Temp == 0u)
            {
                GPIO_togglePin(pLed[i]->Index);
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
            GPIO_writePin(pLed[i]->Index, (uint32_t)pLed[i]->State);
        }
    }
}


/*
@funtion    void setLedStatus(stLed *pLed, uint16_t State)
@brief      개별 LED 상태(점등/소등) 강제 설정
@param      pLed: 대상 LED 구조체 포인터
@param      State: LED_ON(0) 또는 LED_OFF(1)
@return     void
*/
void setLedStatus(stLed *pLed, uint16_t State)
{
    if(pLed != NULL)
    {
        if(pLed->State != State)
        {
            pLed->State = State;
            pLed->Toggle = LED_NONE; 
            GPIO_writePin(pLed->Index, (uint32_t)State);
        }
    }
}


/*
@funtion    void setLedModeToggle(stLed *pLed, uint16_t State, uint16_t Time)
@brief      LED 토글 모드 설정
@param      pLed: 대상 LED 구조체 포인터
@param      State: LED_TOGGLE(1) 또는 LED_NONE(0)
@param      Time: 토글 유지 카운트 (100ms 단위)
@return     void
*/
void setLedModeToggle(stLed *pLed, uint16_t State, uint16_t Time)
{
    if(pLed != NULL)
    {
        pLed->Toggle = State;
        pLed->Time   = Time;
        pLed->Temp   = 0u;
    }
}




