/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_EPWM.c
    Description      : EPWM 7A Control
    Last Updated     : 2026. 05. 22.
**********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "CSU_EPWM.h"


/* ************************** [[   define   ]]  *********************************************************** */



/* ************************** [[   global   ]]  *********************************************************** */
// 이전 설정 상태 보관
static uint16_t prevFreq = 0xFFFF;
static uint16_t prevDuty = 0xFFFF;
static bool prevEn = false;


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
    
    // 초기 구조체 설정: 꺼짐, Duty 50, Freq 1 (100Hz)
    xRcvSciPcMsg1.Command.bit.Epwm7aEn = 0;
    xRcvSciPcMsg1.Epwm7aDuty = 50u;
    xRcvSciPcMsg1.Epwm7aFreq = 1u;

    // 클럭 동기화 (ePWM7 활성화)
    EALLOW;
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC); 
    EDIS;
}

/**
 * @brief SCI_PC 메시지에 따른 EPWM7A 상태 및 주파수, Duty 업데이트
 */
void updateEpwm7aStatus(void)
{
    if (xRcvSciPcMsg1.Command.bit.Epwm7aEn) 
    {
        if (!prevEn) 
        {
            // Enable output (해제)
            EPWM_setActionQualifierContSWForceAction(EPWM7_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_SW_DISABLED); // Resume normal PWM
        }
        
        if ((xRcvSciPcMsg1.Epwm7aFreq != prevFreq) || (xRcvSciPcMsg1.Epwm7aDuty != prevDuty)) 
        {
            uint32_t prd = 0;
            EPWM_ClockDivider clkdiv = EPWM_CLOCK_DIVIDER_128;
            EPWM_HSClockDivider hspclkdiv = EPWM_HSCLOCK_DIVIDER_1;

            switch (xRcvSciPcMsg1.Epwm7aFreq) {
                case 0: // 10Hz
                    hspclkdiv = EPWM_HSCLOCK_DIVIDER_14; // /14
                    clkdiv = EPWM_CLOCK_DIVIDER_128;    // /128
                    prd = 11160;
                    break;
                case 1: // 100Hz
                    hspclkdiv = EPWM_HSCLOCK_DIVIDER_1;  // /1
                    clkdiv = EPWM_CLOCK_DIVIDER_128;    // /128
                    prd = 15624;
                    break;
                case 2: // 1kHz
                    hspclkdiv = EPWM_HSCLOCK_DIVIDER_1;  // /1
                    clkdiv = EPWM_CLOCK_DIVIDER_8;      // /8
                    prd = 24999;
                    break;
                case 3: // 10kHz
                    hspclkdiv = EPWM_HSCLOCK_DIVIDER_1;  // /1
                    clkdiv = EPWM_CLOCK_DIVIDER_1;      // /1
                    prd = 19999;
                    break;
                case 4: // 100kHz
                    hspclkdiv = EPWM_HSCLOCK_DIVIDER_1;  // /1
                    clkdiv = EPWM_CLOCK_DIVIDER_1;      // /1
                    prd = 1999;
                    break;
                case 5: // 1MHz
                    hspclkdiv = EPWM_HSCLOCK_DIVIDER_1;  // /1
                    clkdiv = EPWM_CLOCK_DIVIDER_1;      // /1
                    prd = 199;
                    break;
                case 6: // 10MHz
                    hspclkdiv = EPWM_HSCLOCK_DIVIDER_1;  // /1
                    clkdiv = EPWM_CLOCK_DIVIDER_1;      // /1
                    prd = 19;
                    break;
                default:
                    hspclkdiv = EPWM_HSCLOCK_DIVIDER_1;  // /1
                    clkdiv = EPWM_CLOCK_DIVIDER_128;    // /128
                    prd = 15624;   // Default 100Hz
                    break;
            }

            EPWM_setClockPrescaler(EPWM7_BASE, clkdiv, hspclkdiv);
            EPWM_setTimeBasePeriod(EPWM7_BASE, (uint16_t)prd);
            
            // Duty calculation (1-100%)
            uint16_t duty = xRcvSciPcMsg1.Epwm7aDuty;
            if (duty > 100) duty = 100;
            if (duty < 1)   duty = 1;
            
            // CAU=AQ_CLEAR, ZRO=AQ_SET 이므로, CMPA 시간(0~CMPA)이 High 유지 시간.
            uint32_t cmpa = ((prd + 1) * duty) / 100;
            if (cmpa > prd) cmpa = prd + 1; // 100% duty (항상 켜져있음)
            
            EPWM_setCounterCompareValue(EPWM7_BASE, EPWM_COUNTER_COMPARE_A, (uint16_t)cmpa);

            prevFreq = xRcvSciPcMsg1.Epwm7aFreq;
            prevDuty = xRcvSciPcMsg1.Epwm7aDuty;
        }
    } 
    else 
    {
        if (prevEn) 
        {
            EPWM_setActionQualifierContSWForceAction(EPWM7_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_SW_OUTPUT_LOW); // Force Low
        }
    }
    prevEn = xRcvSciPcMsg1.Command.bit.Epwm7aEn;
}
