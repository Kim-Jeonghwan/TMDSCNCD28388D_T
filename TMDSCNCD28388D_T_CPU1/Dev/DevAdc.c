/**********************************************************************
    Nexcom Co., Ltd.
    Copyright 2021. All Rights Reserved.

    Filename        : DevAdc.c
    Version         : 00.01
    Description     : ADC Driver (Focus on internal Temperature Sensor & EPWM9 Trigger)
    Tracebility     :
    Programmer      :
    Last Updated    : 2026. 06. 05. (코드 주석 포맷팅 및 한글화)

    Function List   :
                      void InitialAdc(void)
                      void InitAdcModules(void)
                      void initEPWM8(void)
                      void initEPWM9(void)
                      interrupt void AdcaIsr(void)
**********************************************************************/

/* ************************** [[   include  ]] *********************************************************** */
#include "DevAdc.h"



/* ************************** [[  static prototype  ]] *************************************************** */
static void setupEPWM8_TimeBase(uint16_t prd);
static void setupEPWM8_ActionQualifier(void);
static void setupEPWM8_AdcTrigger(void);
static void setupEPWM9_TimeBase(uint16_t prd);
static void setupEPWM9_AdcTrigger(void);

/* ************************** [[   define   ]] *********************************************************** */
#define DEFAULT_MAVE_COUNT  100u   // 이동 평균 필터 카운트
#define DEFAULT_PWM_HZ      100000u // ePWM8 트리거 주파수 (100kHz 조정)

/* ************************** [[   global   ]] *********************************************************** */
uint16_t adcResult = 0u; // 실시간 온도 센서 원시 결과 전역 변수 (CSU_Adc.c에서 참조)

/* ************************** [[  function  ]] *********************************************************** */

/*
@funtion    void InitialAdc(void)
@brief      ADC 초기화 기동 및 인터럽트 등록
@param      void
@return     void
*/
void InitialAdc(void)
{
    InitAdcModules(); // ADC 모듈 하드웨어 초기화

    // ADC 인터럽트 등록 및 활성화 (원래 주기 동작 복원)
    Interrupt_register(INT_ADCA1, &AdcaIsr);
    Interrupt_enable(INT_ADCA1);
}

/*
@funtion    void InitAdcModules(void)
@brief      ADC 모듈 초기 설정 (Driverlib 적용)
@param      void
@return     void
*/
void InitAdcModules(void)
{
    // -------------------------------------------------------------------------
    // 1. ADC-A 초기화 (주기 계측 및 내부 온도 센서 수집 담당)
    // -------------------------------------------------------------------------
    EALLOW;
    ADC_setPrescaler(ADCA_BASE, ADC_CLK_DIV_4_0); // ADCCLK = SYSCLK / 4 (50MHz)
    ADC_setMode(ADCA_BASE, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);
    ADC_setInterruptPulseMode(ADCA_BASE, ADC_PULSE_END_OF_CONV);
    ADC_enableConverter(ADCA_BASE);
    DEVICE_DELAY_US(1000U); // 아날로그 회로 파워업 대기

    // ADCA SOC 매핑 설정
    ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM8_SOCA, ADC_CH_ADCIN2, 14u);  // SOC0: A2 (주기 구동용)
    ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER1, ADC_TRIGGER_EPWM8_SOCA, ADC_CH_ADCIN4, 14u);  // SOC1: A4 (주기 구동용)
    
    // SOC2: 내부 온도 센서 (ePWM9_SOCA 자동 트리거 연동, 250 샘플링 윈도우 확보)
    ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_EPWM9_SOCA, ADC_CH_ADCIN13, 250u);

    // SOC0, SOC1, SOC2 중 최종 채널인 SOC2의 변환 완료 시점 기준 인터럽트 INT1 발생
    ADC_setInterruptSource(ADCA_BASE, ADC_INT_NUMBER1, ADC_SOC_NUMBER2);
    ADC_enableInterrupt(ADCA_BASE, ADC_INT_NUMBER1);
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
    EDIS;

    // -------------------------------------------------------------------------
    // 2. ADC-B 초기화 (기본 설정)
    // -------------------------------------------------------------------------
    EALLOW;
    ADC_setPrescaler(ADCB_BASE, ADC_CLK_DIV_8_0);
    ADC_setMode(ADCB_BASE, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);
    ADC_enableConverter(ADCB_BASE);
    DEVICE_DELAY_US(1000U);
    EDIS;

    // -------------------------------------------------------------------------
    // 3. ADC-C 초기화 (기본 설정 및 안정화)
    // -------------------------------------------------------------------------
    EALLOW;
    ADC_setPrescaler(ADCC_BASE, ADC_CLK_DIV_4_0);
    ADC_setMode(ADCC_BASE, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);
    ADC_enableConverter(ADCC_BASE);
    DEVICE_DELAY_US(1000u);
    EDIS;

    // -------------------------------------------------------------------------
    // 4. ADC-D 초기화 (기본 설정 및 SOC0 매핑)
    // -------------------------------------------------------------------------
    EALLOW;
    ADC_setPrescaler(ADCD_BASE, ADC_CLK_DIV_4_0);
    ADC_setMode(ADCD_BASE, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);
    ADC_enableConverter(ADCD_BASE);
    DEVICE_DELAY_US(1000U);

    ADC_setupSOC(ADCD_BASE, ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM8_SOCA, ADC_CH_ADCIN4, 14u); // SOC0: D4
    EDIS;
}

/*
@funtion    void initEPWM8(void)
@brief      ePWM8 모듈 초기화 (ADC 하드웨어 동기 트리거 소스)
@param      void
@return     void
*/
void initEPWM8(void)
{
    EALLOW;
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM8); // ePWM8 클럭 인가

    // ePWM8 타임베이스 주기 연산 (100kHz 타깃)
    uint32_t prd = SYSCLK / ((uint32_t)DEFAULT_PWM_HZ * 4u); 

    setupEPWM8_TimeBase((uint16_t)prd);
    setupEPWM8_ActionQualifier();
    setupEPWM8_AdcTrigger();

    // ePWM 타임베이스 클럭 전역 동기화 및 기동
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC); 
    EDIS;
}

/*
@funtion    static void setupEPWM8_TimeBase(uint16_t prd)
@brief      ePWM8 타임 베이스 주파수 및 분주 설정
@param      prd: 계산된 주기 값
@return     static void
*/
static void setupEPWM8_TimeBase(uint16_t prd)
{
    EPWM_setTimeBasePeriod(EPWM8_BASE, prd);
    EPWM_setPhaseShift(EPWM8_BASE, 0u);
    EPWM_setTimeBaseCounter(EPWM8_BASE, 0u);

    EPWM_setTimeBaseCounterMode(EPWM8_BASE, EPWM_COUNTER_MODE_UP); // 업 카운트 모드
    EPWM_disablePhaseShiftLoad(EPWM8_BASE);
    // 기존 DIV_2는 수식(prd = SYSCLK / (Hz * 4))과 불일치하여 2배 빠르게 도는 버그가 있었음. 
    // DIV_4로 수정하여 주파수를 100kHz에 완벽하게 일치시킴.
    EPWM_setClockPrescaler(EPWM8_BASE, EPWM_CLOCK_DIVIDER_4, EPWM_HSCLOCK_DIVIDER_1); // 클럭 분주 /4

    // 50% 듀티 기준 비교값 설정
    EPWM_setCounterCompareValue(EPWM8_BASE, EPWM_COUNTER_COMPARE_A, (uint16_t)(prd / 2u));
}

/*
@funtion    static void setupEPWM8_ActionQualifier(void)
@brief      ePWM8 비교 매칭 액션 퀄리파이어 설정
@param      void
@return     static void
*/
static void setupEPWM8_ActionQualifier(void)
{
    EPWM_setActionQualifierAction(EPWM8_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(EPWM8_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
}

/*
@funtion    static void setupEPWM8_AdcTrigger(void)
@brief      ePWM8 ADC SOCA 발생 이벤트 설정
@param      void
@return     static void
*/
static void setupEPWM8_AdcTrigger(void)
{
    EPWM_enableADCTrigger(EPWM8_BASE, EPWM_SOC_A);
    EPWM_setADCTriggerSource(EPWM8_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_PERIOD); // 주기에 도달할 때 SOCA 펄스 발생
    EPWM_setADCTriggerEventPrescale(EPWM8_BASE, EPWM_SOC_A, 1u);            // 1회 매칭당 1회 트리거
}

/*
@funtion    void initEPWM9(void)
@brief      ePWM9 모듈 초기화 (온도 센서 전용 1kHz 느린 트리거)
@param      void
@return     void
*/
void initEPWM9(void)
{
    EALLOW;
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM9); // ePWM9 클럭 인가

    // ePWM9 타임베이스 주기 연산 (느리고 안전한 1kHz 설정)
    uint32_t prd = SYSCLK / (1000u * 4u); 

    setupEPWM9_TimeBase((uint16_t)prd);
    setupEPWM9_AdcTrigger();

    // ePWM 타임베이스 클럭 전역 동기화 및 기동
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC); 
    EDIS;
}

/*
@funtion    static void setupEPWM9_TimeBase(uint16_t prd)
@brief      ePWM9 타임 베이스 주파수 및 분주 설정
@param      prd: 계산된 주기 값
@return     static void
*/
static void setupEPWM9_TimeBase(uint16_t prd)
{
    EPWM_setTimeBasePeriod(EPWM9_BASE, prd);
    EPWM_setPhaseShift(EPWM9_BASE, 0u);
    EPWM_setTimeBaseCounter(EPWM9_BASE, 0u);

    EPWM_setTimeBaseCounterMode(EPWM9_BASE, EPWM_COUNTER_MODE_UP); // 업 카운트 모드
    EPWM_disablePhaseShiftLoad(EPWM9_BASE);
    // DIV_4로 수정하여 온도 센서용 ADC 트리거 주파수를 정확한 1kHz로 교정
    EPWM_setClockPrescaler(EPWM9_BASE, EPWM_CLOCK_DIVIDER_4, EPWM_HSCLOCK_DIVIDER_1); // 클럭 분주 /4
}

/*
@funtion    static void setupEPWM9_AdcTrigger(void)
@brief      ePWM9 ADC SOCA 발생 이벤트 설정
@param      void
@return     static void
*/
static void setupEPWM9_AdcTrigger(void)
{
    EPWM_enableADCTrigger(EPWM9_BASE, EPWM_SOC_A);
    EPWM_setADCTriggerSource(EPWM9_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_PERIOD); // 주기에 도달할 때 SOCA 펄스 발생
    EPWM_setADCTriggerEventPrescale(EPWM9_BASE, EPWM_SOC_A, 1u);            // 1회 매칭당 1회 트리거
}

/*
@funtion    interrupt void AdcaIsr(void)
@brief      ADCINA1 인터럽트 서비스 루틴 (백그라운드 실시간 초고속 데이터 취득)
@param      void
@return     interrupt void
*/
interrupt void AdcaIsr(void)
{
    // SOC2 (내부 온도 센서 채널 13) 변환 결과 실시간 취득
    adcResult = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER2);

    // 인터럽트 오버플로우(Interrupt Overflow) 감지 시 강제 해제하여 ADC 락업 방어 (CWE-658 방어 규격 준수)
    if (ADC_getInterruptOverflowStatus(ADCA_BASE, ADC_INT_NUMBER1))
    {
        ADC_clearInterruptOverflowStatus(ADCA_BASE, ADC_INT_NUMBER1);
    }

    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
