/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Timer.h
    Description      : CM Core SysTick 타이머 헤더
    Last Updated     : 2026. 06. 05. (코드 주석 포맷팅 및 한글화)
**********************************************************************/
#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include "main_cm.h"


typedef struct
{
    uint16_t Cycle_2ms;    // 2ms 주기 - UDP 송신 전용 (Timer0)
    uint16_t Cycle_1ms;
    uint16_t Cycle_10ms;
    uint16_t Cycle_100ms;
    uint16_t Cycle_1000ms;

    uint16_t Hzcnt;
    uint16_t Hz;
} stTimer;

extern volatile stTimer xTimer;

// 타이머 초기화
extern void Initial_TIMER(void);

// CPU 타이머 인터럽트 핸들러
extern void isr_CpuTimer0(void); // 이더넷 송신
extern void isr_CpuTimer1(void); // 주기적 작업
extern void isr_CpuTimer2(void); // Hz 측정

#endif // HAL_TIMER_H
