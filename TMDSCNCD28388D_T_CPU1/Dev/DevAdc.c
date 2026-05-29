/**********************************************************************

	Nexcom Co., Ltd.
	Copyright 2021. All Rights Reserved.

	Filename		: DevAdc.c
	Version			: 00.00
	Description		: ADC Driver (Focus on ADCINA2)
	Tracebility		: 
	Programmer		: 
	Last Updated	: 2026. 05. 29. (정적 시험 기준 준수 및 보안 취약점 보완)

	Function List	:	
						

**********************************************************************/

/*
 * Modification History
 * --------------------
 * 
 * 
*/


/* DESCRIPTION
 * 
 * 
*/


/* ************************** [[   include  ]]  *********************************************************** */
#include "DevAdc.h"
#include "CSU_Adc.h"

/* ************************** [[  static prototype  ]]  *************************************************** */
static void setupEPWM8_TimeBase(uint16_t prd);
static void setupEPWM8_ActionQualifier(void);
static void setupEPWM8_AdcTrigger(void);

/* ************************** [[   define   ]]  *********************************************************** */
#define DEFAULT_MAVE_COUNT   100u   // 이동 평균 필터 카운트
#define DEFAULT_PWM_HZ       100000u  // ePWM8 트리거 주파수 (100kHz 조정)


/* ************************** [[   global   ]]  *********************************************************** */
float32_t lpf_PrevValue   = 0.0f;     // ADC LPF 이전 값
float32_t lpf_PrevPWM7a   = 0.0f;     // ADC LPF 이전 값 (PWM7a)
float32_t f32PotenRAW     = 0.0f;     // Poten RAW 결과 (float) 
float32_t f32PotenMAVE    = 0.0f;     // Poten MAVE 결과 (float)
float32_t f32PWMRaw       = 0.0f;     // PWM Raw 결과 (float)
float32_t f32PWMRCLPF     = 0.0f;     // PWM RC LPF 결과 (float)
float32_t f32PWMBWLPF     = 0.0f;     // PWM BW LPF 결과 (float)
float32_t f32PotenSum      = 0.0f;     // Poten 누적 합 (Running Sum용)


uint16_t  ResultsIndex    = 0u;       // ADC 결과 인덱스

float32_t AdcResults[BUFF_SIZE];      // ADC 결과 저장 버퍼

/* ************************** [[  function  ]]  *********************************************************** */

// FLASH 메모리에서 실행할 경우, 인터럽트 서비스 루틴을 램(RAM) 영역에서 실행하기 위한 설정
#ifdef _FLASH
    #pragma CODE_SECTION(AdcaIsr, ".TI.ramfunc");
#endif


/*
@funtion    interrupt void AdcaIsr(void)
@brief      ADC-A 인터럽트 서비스 루틴 (ADCINA2 사용)
@param      void
@return     interrupt void
@remark
    - 10kHz 주기로 실행 (ePWM8 트리거)
*/
interrupt void AdcaIsr(void)
{
    // 1. 실시간 전압 판독 및 저장 (10kHz) - Driverlib 적용
    f32PotenRAW  = (float32_t)ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0) * CONV_ADC_3_3V; // A2
    f32PWMRaw    = (float32_t)ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER1) * CONV_ADC_3_3V; // A4
    f32PWMRCLPF  = (float32_t)ADC_readResult(ADCCRESULT_BASE, ADC_SOC_NUMBER0) * CONV_ADC_3_3V; // C4
    f32PWMBWLPF  = (float32_t)ADC_readResult(ADCDRESULT_BASE, ADC_SOC_NUMBER0) * CONV_ADC_3_3V; // D4

    xTimer.Hzcnt++;

    // 2. Poten MAVE 최적화 로직 (Running Sum 방식 - O(1))
    // 루프를 돌지 않고 가장 오래된 값을 빼고 새 값을 더해 효율성 극대화
    f32PotenSum -= AdcResults[ResultsIndex];
    f32PotenSum += f32PotenRAW;
    AdcResults[ResultsIndex++] = f32PotenRAW;

    if(ResultsIndex >= DEFAULT_MAVE_COUNT)
    {
        ResultsIndex = 0u;
    }

    // 평균값 업데이트 (단일 나눗셈만 수행)
    f32PotenMAVE = f32PotenSum / (float32_t)DEFAULT_MAVE_COUNT;


    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);      // ADC-A 인터럽트 플래그(ADCINT1) 클리어

    // ADCINT1 이벤트 플래그 오버플로우 확인 및 클리어
    if(ADC_getInterruptOverflowStatus(ADCA_BASE, ADC_INT_NUMBER1))
    {
        ADC_clearInterruptOverflowStatus(ADCA_BASE, ADC_INT_NUMBER1); // 인터럽트 오버플로우 플래그 클리어
        ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);        // 인터럽트 플래그 재클리어
    }

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);     // PIE 인터럽트 승인 (Group 1)
}



/*
@funtion    void InitialAdc(void)
@brief      ADC 초기화 시작
@param      void
@return     void
@remark
    -
*/
void InitialAdc(void)
{
    InitAdcModules();       // ADC 모듈 하드웨어 초기화

    // Driverlib 적용 인터럽트 등록 및 활성화
    Interrupt_register(INT_ADCA1, &AdcaIsr);
    Interrupt_enable(INT_ADCA1);
}


/*
@funtion    void InitAdcModules(void)
@brief      ADC 모듈 초기 설정 (Driverlib 적용)
@param      void
@return     void
@remark
    -
*/
void InitAdcModules(void)
{
    // ADC-A 초기화
    EALLOW;
    ADC_setPrescaler(ADCA_BASE, ADC_CLK_DIV_4_0);	// ADCCLK = SYSCLK / 4 (200MHz / 4 = 50MHz : 최대 속도)
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    ADC_setInterruptPulseMode(ADCA_BASE, ADC_PULSE_END_OF_CONV);
    ADC_enableConverter(ADCA_BASE);
    DELAY_US(1000);
    
    // SOC 설정
    ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM8_SOCA, ADC_CH_ADCIN2, 14u); // SOC0: A2
    ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER1, ADC_TRIGGER_EPWM8_SOCA, ADC_CH_ADCIN4, 14u); // SOC1: A4

    ADC_setInterruptSource(ADCA_BASE, ADC_INT_NUMBER1, ADC_SOC_NUMBER0); // SOC0 변환 완료 시 INT1 시퀀스 발생
    ADC_enableInterrupt(ADCA_BASE, ADC_INT_NUMBER1);                    // ADCINT1 인터럽트 활성화
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);                // 인터럽트 플래그 클리어
    EDIS;

	// ADC-B 초기화
    EALLOW;
    ADC_setPrescaler(ADCB_BASE, ADC_CLK_DIV_8_0);
    AdcSetMode(ADC_ADCB, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    ADC_enableConverter(ADCB_BASE);
    DELAY_US(1000);
    EDIS;


	// ADC-C 초기화
    EALLOW;
    ADC_setPrescaler(ADCC_BASE, ADC_CLK_DIV_4_0);
    AdcSetMode(ADC_ADCC, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    ADC_enableConverter(ADCC_BASE);
    DELAY_US(1000);

    ADC_setupSOC(ADCC_BASE, ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM8_SOCA, ADC_CH_ADCIN4, 14u); // SOC0: C4
    EDIS;


	// ADC-D 초기화
    EALLOW;
    ADC_setPrescaler(ADCD_BASE, ADC_CLK_DIV_4_0);
    AdcSetMode(ADC_ADCD, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    ADC_enableConverter(ADCD_BASE);
    DELAY_US(1000);

    ADC_setupSOC(ADCD_BASE, ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM8_SOCA, ADC_CH_ADCIN4, 14u); // SOC0: D4
    EDIS;
}



/*
@funtion    float32_t low_pass_filter(float32_t input, float32_t alpha, float32_t *prev_output)
@brief      로우 패스 필터 (LPF) 구현
@param      float32_t input: 입력 값
@param      float32_t alpha: 필터 계수 (0~1)
@param      float32_t *prev_output: 이전 출력 값 저장 포인터
@return     float32_t: 필터링된 결과 값
@remark
    -
*/
float32_t low_pass_filter(float32_t input, float32_t alpha, float32_t *prev_output)
{
    float32_t output = input;
    if (prev_output != NULL)
    {
        output = alpha * input + (1.0f - alpha) * (*prev_output);
        *prev_output = output;
    }
    return output;
}

/*
@funtion    float32_t Within_f32(float32_t val, float32_t min, float32_t max)
@brief      값의 범위를 제한 (Saturation)
@param      float32_t val: 입력 값
@param      float32_t min: 최솟값
@param      float32_t max: 최댓값
@return     float32_t: 제한된 범위 내의 결과
@remark
    -
*/
float32_t Within_f32(float32_t val, float32_t min, float32_t max)
{
    float32_t result = val;
    
    if (val <= min)
    {
        result = min;
    }
    else if (val >= max)
    {
        result = max;
    }
    else
    {
        // DAPA SCR-G 예외 방어용 블록
    }
    
    return result;
}


/*
@funtion    void initEPWM8(void)
@brief      ePWM8 모듈 초기화 (ADC 트리거 용도)
@param      void
@return     void
@remark
    -
*/
void initEPWM8(void)
{
    EALLOW;
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM8); // ePWM8 클럭 활성화
    
    // ePWM8 기본 설정 - Driverlib 적용
    uint32_t prd = SYSCLK / ((uint32_t)DEFAULT_PWM_HZ * 4u); // 주기 설정 (예: 500 타임베이스 = 100kHz)
    
    setupEPWM8_TimeBase((uint16_t)prd);
    setupEPWM8_ActionQualifier();
    setupEPWM8_AdcTrigger();
    
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC); // ePWM 타임베이스 클럭 동기화 (카운터 시작)
    EDIS;
}


/*
@funtion    static void setupEPWM8_TimeBase(uint16_t prd)
@brief      ePWM8 타임 베이스 및 분주 설정
@param      uint16_t prd: 주기 값
@return     static void
*/
static void setupEPWM8_TimeBase(uint16_t prd)
{
    EPWM_setTimeBasePeriod(EPWM8_BASE, prd);
    EPWM_setPhaseShift(EPWM8_BASE, 0u);                      // 위상 0 설정
    EPWM_setTimeBaseCounter(EPWM8_BASE, 0u);                 // 타임베이스 카운터 초기화

    EPWM_setTimeBaseCounterMode(EPWM8_BASE, EPWM_COUNTER_MODE_UP); // 타임베이스 카운트 모드: 업 카운트
    EPWM_disablePhaseShiftLoad(EPWM8_BASE);                       // 위상 로드 비활성화
    EPWM_setClockPrescaler(EPWM8_BASE, EPWM_CLOCK_DIVIDER_2, EPWM_HSCLOCK_DIVIDER_1); // 클럭 분주 (/2, /1 = 전체 /2)

    // 비교값 설정
    EPWM_setCounterCompareValue(EPWM8_BASE, EPWM_COUNTER_COMPARE_A, (uint16_t)(prd / 2u)); // 50% 듀티 사이클 설정
}


/*
@funtion    static void setupEPWM8_ActionQualifier(void)
@brief      ePWM8 액션 퀄리파이어 설정
@param      void
@return     static void
*/
static void setupEPWM8_ActionQualifier(void)
{
    // ePWM 액션 한정기(Action Qualifier) 설정
    EPWM_setActionQualifierAction(EPWM8_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA); // CMPA와 일치하면 출력 High
    EPWM_setActionQualifierAction(EPWM8_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);    // 카운터가 0이면 출력 Low
}


/*
@funtion    static void setupEPWM8_AdcTrigger(void)
@brief      ePWM8 ADC SOCA 트리거 설정
@param      void
@return     static void
*/
static void setupEPWM8_AdcTrigger(void)
{
    // ADC 트리거 설정 (SOCA)
    EPWM_enableADCTrigger(EPWM8_BASE, EPWM_SOC_A);                     // SOCA 활성화
    EPWM_setADCTriggerSource(EPWM8_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_PERIOD); // 카운터가 주기(TBPRD)에 도달할 때 SOCA 발생
    EPWM_setADCTriggerEventPrescale(EPWM8_BASE, EPWM_SOC_A, 1u);       // 첫 번째 이벤트마다 SOCA 발생
}
