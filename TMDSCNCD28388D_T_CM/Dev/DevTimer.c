/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevTimer.c
    Description      : CM Core CPU 타이머 소스
    Last Updated     : 2026. 05. 29. (정적 시험 기준 준수 및 함수 주석 보강)
**********************************************************************/

#include "DevTimer.h"

stTimer xTimer;

/*
@funtion    void Initial_TIMER(void)
@brief      CM 코어의 CPU 타이머 0, 1, 2 모듈 초기화 및 인터럽트 활성화
@param      void
@return     void
@remark
    - 타이머 0(1ms), 타이머 1(1ms), 타이머 2(1000ms)를 각각의 전용 주기에 따라 기동하고 인터럽트를 구성합니다.
*/
void Initial_TIMER(void)
{
    // 타이머 변수 초기화
    (void)memset(&xTimer, 0, sizeof(xTimer));

    // --- CPU 타이머 0 초기화 (이더넷 송신용: 1ms) ---
    CPUTimer_setPeriod(CPUTIMER0_BASE, 125000U); // 1ms 주기 설정 @ 125MHz
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0U);
    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    Interrupt_registerHandler(INT_TIMER0, isr_CpuTimer0);
    CPUTimer_enableInterrupt(CPUTIMER0_BASE);
    CPUTimer_startTimer(CPUTIMER0_BASE);
    Interrupt_enable(INT_TIMER0);

    // --- CPU 타이머 1 초기화 (주기적 작업용: 1ms) ---
    CPUTimer_setPeriod(CPUTIMER1_BASE, 125000U); // 1ms 주기 설정 @ 125MHz
    CPUTimer_setPreScaler(CPUTIMER1_BASE, 0U);
    CPUTimer_stopTimer(CPUTIMER1_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER1_BASE);
    Interrupt_registerHandler(INT_TIMER1, isr_CpuTimer1);
    CPUTimer_enableInterrupt(CPUTIMER1_BASE);
    CPUTimer_startTimer(CPUTIMER1_BASE);
    Interrupt_enable(INT_TIMER1);

    // --- CPU 타이머 2 초기화 (Hz 측정용: 1000ms = 1s) ---
    CPUTimer_setPeriod(CPUTIMER2_BASE, 125000000U); // 1초 주기 설정 @ 125MHz
    CPUTimer_setPreScaler(CPUTIMER2_BASE, 0U);
    CPUTimer_stopTimer(CPUTIMER2_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER2_BASE);
    Interrupt_registerHandler(INT_TIMER2, isr_CpuTimer2);
    CPUTimer_enableInterrupt(CPUTIMER2_BASE);
    CPUTimer_startTimer(CPUTIMER2_BASE);
    Interrupt_enable(INT_TIMER2);
}

// 타이머 0: 이더넷 송신 핸들러 (1ms)
/*
@funtion    void isr_CpuTimer0(void)
@brief      CPU 타이머 0 인터럽트 서비스 루틴 (1ms)
@param      void
@return     void
@remark
    - 타이머 0 오버플로우 플래그를 클리어합니다. (이더넷 송신 태스크 등 활용 가능)
*/
void isr_CpuTimer0(void)
{
    // 타이머 플래그 클리어
    CPUTimer_clearOverflowFlag(CPUTIMER0_BASE);
}

// 타이머 1: 주기적 작업 핸들러 (1ms)
/*
@funtion    void isr_CpuTimer1(void)
@brief      CPU 타이머 1 인터럽트 서비스 루틴 (1ms)
@param      void
@return     void
@remark
    - 소프트웨어 스케줄링용 카운터들(1ms, 10ms, 100ms, 1000ms)을 누적 증가시키고 플래그를 클리어합니다.
*/
void isr_CpuTimer1(void)
{
    xTimer.Cycle_1ms++;
    xTimer.Cycle_10ms++;
    xTimer.Cycle_100ms++;
    xTimer.Cycle_1000ms++;

    // 타이머 플래그 클리어
    CPUTimer_clearOverflowFlag(CPUTIMER1_BASE);
}

// 타이머 2: Hz 측정 핸들러 (1000ms = 1s)
/*
@funtion    void isr_CpuTimer2(void)
@brief      CPU 타이머 2 인터럽트 서비스 루틴 (1000ms = 1s)
@param      void
@return     void
@remark
    - 1초 간 누적된 클럭 연산 루프 카운트(Hzcnt)를 토대로 동작 주파수(Hz)를 측정하고 플래그를 소거합니다.
*/
void isr_CpuTimer2(void)
{
    xTimer.Hz = xTimer.Hzcnt;
    xTimer.Hzcnt = 0;

    // 타이머 플래그 클리어
    CPUTimer_clearOverflowFlag(CPUTIMER2_BASE);
}
