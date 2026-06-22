/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Timer.c
    Version          : 00.01
    Description      : CM Core CPU 타이머 소스
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (매크로 상수 헤더로 이동 및 작성자 기입)
 * 2026. 06. 05. - 코드 주석 포맷팅 및 한글화
 */

#include "hal_Timer.h"

volatile stTimer xTimer;

/*
@funtion    void Initial_TIMER(void)
@brief      CM 코어의 CPU 타이머(0, 1, 2)를 초기화합니다.
@param      void
@return     void
@remark
    - 타이머 0: 2ms 주기, UDP 이더넷 송신 주기 생성용
    - 타이머 1: 1ms 주기, 백그라운드 태스크 스케줄링용
    - 타이머 2: 1s 주기, 디버깅용 Hz 계수
*/
void Initial_TIMER(void)
{
    /* 타이머 변수 초기화 (volatile 경고 방지를 위해 명시적 형변환) */
    (void)memset((void *)&xTimer, 0U, sizeof(xTimer));

    /* --- CPU 타이머 0: UDP 이더넷 송신 전용 (2ms) --- */
    CPUTimer_setPeriod(CPUTIMER0_BASE, TIMER0_PERIOD_2MS - 1U);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0U);
    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    
    // hw_ints.h 규격에 맞게 표준 매크로(INT_TIMER0) 사용
    Interrupt_registerHandler(INT_TIMER0, isr_CpuTimer0);
    CPUTimer_enableInterrupt(CPUTIMER0_BASE);
    CPUTimer_startTimer(CPUTIMER0_BASE);
    Interrupt_enable(INT_TIMER0);

    /* --- CPU 타이머 1: 주기적 작업 스케줄러 (1ms) --- */
    CPUTimer_setPeriod(CPUTIMER1_BASE, TIMER1_PERIOD_1MS - 1U);
    CPUTimer_setPreScaler(CPUTIMER1_BASE, 0U);
    CPUTimer_stopTimer(CPUTIMER1_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER1_BASE);
    
    // hw_ints.h 규격에 맞게 표준 매크로(INT_TIMER1) 사용
    Interrupt_registerHandler(INT_TIMER1, isr_CpuTimer1);
    CPUTimer_enableInterrupt(CPUTIMER1_BASE);
    CPUTimer_startTimer(CPUTIMER1_BASE);
    Interrupt_enable(INT_TIMER1);

    /* --- CPU 타이머 2: Hz 측정용 (1000ms = 1s) --- */
    CPUTimer_setPeriod(CPUTIMER2_BASE, TIMER2_PERIOD_1S - 1U);
    CPUTimer_setPreScaler(CPUTIMER2_BASE, 0U);
    CPUTimer_stopTimer(CPUTIMER2_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER2_BASE);
    
    // hw_ints.h 규격에 맞게 표준 매크로(INT_TIMER2) 사용
    Interrupt_registerHandler(INT_TIMER2, isr_CpuTimer2);
    CPUTimer_enableInterrupt(CPUTIMER2_BASE);
    CPUTimer_startTimer(CPUTIMER2_BASE);
    Interrupt_enable(INT_TIMER2);
}

/*
@funtion    void isr_CpuTimer0(void)
@brief      타이머 0 인터럽트 서비스 루틴 (2ms)
@param      void
@return     void
*/
void isr_CpuTimer0(void)
{
    xTimer.Cycle_2ms++;
    CPUTimer_clearOverflowFlag(CPUTIMER0_BASE);
}

/*
@funtion    void isr_CpuTimer1(void)
@brief      타이머 1 인터럽트 서비스 루틴 (1ms)
@param      void
@return     void
*/
void isr_CpuTimer1(void)
{
    xTimer.Cycle_1ms++;
    xTimer.Cycle_10ms++;
    xTimer.Cycle_100ms++;
    xTimer.Cycle_1000ms++;

    CPUTimer_clearOverflowFlag(CPUTIMER1_BASE);
}

/*
@funtion    void isr_CpuTimer2(void)
@brief      타이머 2 인터럽트 서비스 루틴 (1s)
@param      void
@return     void
*/
void isr_CpuTimer2(void)
{
    xTimer.Hz = xTimer.Hzcnt;
    xTimer.Hzcnt = 0U;

    CPUTimer_clearOverflowFlag(CPUTIMER2_BASE);
}
