/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Epwm.c
    Version          : 00.00
    Description      : EPWM 7A Control (Initialization skeleton)
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (모듈 및 파일명 리팩토링)
**********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "csu_Epwm.h"


/* ************************** [[   define   ]]  *********************************************************** */



/* ************************** [[   global   ]]  *********************************************************** */

/* ************************** [[  function  ]]  *********************************************************** */

/*
@funtion    static void initEpwm7aGpio(void)
@brief      EPWM7A(GPIO12) 핀 설정
@param      void
@return     static void
@remark
    - GPIO 12번 핀을 EPWM7A 출력으로 할당합니다.
*/
static void initEpwm7aGpio(void)
{
    EALLOW;
    GPIO_setPinConfig(GPIO_12_EPWM7A);
    GPIO_setPadConfig(12, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(12, GPIO_DIR_MODE_OUT);
    EDIS;
}

/*
@funtion    void Initial_Epwm7a(void)
@brief      EPWM7A 모듈 초기화
@param      void
@return     void
@remark
    - 기본 설정: 100Hz, 50% Duty 로 구성하되, 최초 기동 시에는 출력 정지(Force Low) 상태로 설정합니다.
*/
void Initial_Epwm7a(void)
{
    initEpwm7aGpio();

    EALLOW;
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM7); // ePWM7 클럭 활성화
    EDIS;

    // 기본 설정: Up-count mode (Driverlib 적용)
    EPWM_setTimeBaseCounterMode(EPWM7_BASE, EPWM_COUNTER_MODE_UP);
    EPWM_disablePhaseShiftLoad(EPWM7_BASE);
    EPWM_disableSyncOutPulseSource(EPWM7_BASE, EPWM_SYNC_OUT_PULSE_ON_ALL); // 동기화 비활성화
    EPWM_setTimeBaseCounter(EPWM7_BASE, 0u);

    // 액션 한정기 설정
    EPWM_setActionQualifierAction(EPWM7_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA); // CMPA 도달 시 Low
    EPWM_setActionQualifierAction(EPWM7_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);   // 0 도달 시 High
    
    // PWM 출력 오프 (Force Low)로 시작
    EPWM_setActionQualifierContSWForceAction(EPWM7_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_SW_OUTPUT_LOW);

    // 클럭 동기화 (ePWM7 활성화)
    EALLOW;
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC); 
    EDIS;
}

