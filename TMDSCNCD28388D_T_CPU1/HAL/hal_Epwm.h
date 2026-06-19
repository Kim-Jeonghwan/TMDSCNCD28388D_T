/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Epwm.h
    Version          : 00.02
    Description      : CPU1 EPWM1 기반 100us 메인 인터럽트 헤더
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (모듈 및 파일명 리팩토링)
**********************************************************************/

#ifndef HAL_EPWM_H
#define HAL_EPWM_H

#include "main_cpu1.h"

/*
 * [EPWM1 타이머 설계]
 *   - DSP CPU 클럭: 200 MHz
 *   - 모드: UP-DOWN (대칭 PWM)
 *   - Period: 10,000 → 주기 = 10,000 × 2 / 200,000,000 = 100us (10kHz)
 *   - Zero Event (카운터 = 0) 시 ISR 발생 → 100us 주기 트리거
 *   - ADC 갱신 및 사인파(Sine wave)를 생성하여 IPC 전송
 */
#define EPWM_TIMER1_BASE       EPWM1_BASE     /* EPWM1 기본 주소 */
#define EPWM_TIMER1_PERIOD     (10000U)       /* 200MHz, UP-DOWN, 100us */
#define EPWM_TIMER1_CLK_DIV    EPWM_CLOCK_DIVIDER_1
#define EPWM_TIMER1_HCLK_DIV   EPWM_HSCLOCK_DIVIDER_1

/* 함수 프로토타입 */
void Initial_EpwmTimer(void);

#endif /* HAL_EPWM_H */
