/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_EPWM.c
    Description      : EPWM 7A Control (Initialization skeleton)
    Last Updated     : 2026. 06. 01. (빌드 오류 방지를 위한 제거된 구조체 멤버 참조 코드 삭제)
**********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "CSU_EPWM.h"


/* ************************** [[   define   ]]  *********************************************************** */



/* ************************** [[   global   ]]  *********************************************************** */

/* ************************** [[  function  ]]  *********************************************************** */

/**
 * @brief EPWM7A(GPIO12) 핀 설정
 */
static void initEpwm7aGpio(void)
{
    EALLOW;
    GPIO_setPinConfig(GPIO_12_EPWM7A);
    GPIO_setPadConfig(12, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(12, GPIO_DIR_MODE_OUT);
    EDIS;
}

/**
 * @brief EPWM7A 초기화 (기본: 100Hz, 50% Duty, 출력 정지)
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

