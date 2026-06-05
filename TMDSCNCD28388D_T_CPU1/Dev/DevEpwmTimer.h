/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevEpwmTimer.h
    Description      : EPWM1 기반 2ms 하드웨어 타이머 헤더
    Last Updated     : 2026. 06. 05. (코드 주석 포맷팅 및 한글화)
**********************************************************************/

#ifndef DEV_EPWM_TIMER_H
#define DEV_EPWM_TIMER_H

#include "main.h"

/*
 * [EPWM1 타이머 설계]
 *   - DSP CPU 클럭: 200 MHz
 *   - 모드: UP-DOWN (대칭 PWM)
 *   - Period: 200,000 → 주기 = 200,000 × 2 / 200,000,000 = 2ms
 *   - Zero Event (카운터 = 0) 시 ISR 발생 → 2ms 주기 트리거
 *   - sendEthDataToCM() 을 ISR에서 호출하여 CM으로 온도/시퀀스 전송
 */
#define EPWM_TIMER1_BASE       EPWM1_BASE     /* EPWM1 기본 주소 */
#define EPWM_TIMER1_PERIOD     (200000U)      /* 200MHz, UP-DOWN, 2ms */
#define EPWM_TIMER1_CLK_DIV    EPWM_CLOCK_DIVIDER_1
#define EPWM_TIMER1_HCLK_DIV   EPWM_HSCLOCK_DIVIDER_1

/* 함수 프로토타입 */
void Initial_EpwmTimer(void);

#endif /* DEV_EPWM_TIMER_H */
