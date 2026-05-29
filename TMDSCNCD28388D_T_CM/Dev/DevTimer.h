/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevTimer.h
    Description      : CM Core SysTick 타이머 헤더
    Last Updated     : 2026. 04. 22.
**********************************************************************/
#ifndef DEVTIMER_H
#define DEVTIMER_H

#include "main.h"


typedef struct
{
    uint16_t Cycle_1ms;
    uint16_t Cycle_10ms;
    uint16_t Cycle_100ms;
    uint16_t Cycle_1000ms;

    uint16_t Hzcnt;
    uint16_t Hz;
} stTimer;

extern stTimer xTimer;

// 타이머 초기화
extern void Initial_TIMER(void);

// CPU 타이머 인터럽트 핸들러
extern void isr_CpuTimer0(void); // 이더넷 송신
extern void isr_CpuTimer1(void); // 주기적 작업
extern void isr_CpuTimer2(void); // Hz 측정

#endif // DEVTIMER_H
