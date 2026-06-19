/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Epwm.c
    Version          : 00.05
    Description      : CPU1 EPWM1 타이머 초기화 (HAL 분리)
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (CSU/HAL 계층 분리: 제어 로직 이전)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - CSU/HAL 계층 분리: isr_Epwm1Timer100us 제어 로직을 csu_Control.c로 완전 이전
 * 2026. 06. 19. - isr_Epwm1Timer100us 내부에 GPIO 34 토글 로직 추가 (ATTLA_T 동기화)
 * 2026. 06. 19. - isr_Epwm1Timer100us 내부에 GPIO 34 토글 주기를 500ms(5000 카운트)로 수정 (육안 점멸 확인 목적)
 * 2026. 06. 19. - Initial_EpwmTimer() 내부 TBCLKSYNC 정지/재가동 추가 (타이머 동기화 버그 수정)
 */

/*
 * [EPWM1 타이머 설계]
 *   - CPU 클럭: 200 MHz
 *   - 모드: UP-DOWN 카운터 (ePWM 대칭 모드)
 *   - Period 레지스터: 200,000
 *   - 실제 주기: (200,000 × 2) / 200,000,000 = 2ms
 *   - Zero 이벤트(카운터 = 0)에서 INT 발생 → 2ms 마다 ISR 호출
 *   - ISR 내에서 sendEthDataToCM() 호출 → CM이 UDP Reflect 패킷 송신
 *
 * [DspTemp 소스]
 *   xXmtSciPcMsg1.DspTemp : csu_SciPc.c 에서 ADC 읽어서 갱신 (x10 스케일)
 *   xXmtSciPcMsg1.IncNumber: 시퀀스 번호
 *   xXmtSciPcMsg1.Status   : 상태 바이트
 */

#include "hal_Epwm.h"



/*
@function    Initial_EpwmTimer
@brief      EPWM1 기반 100us 타이머 초기화 및 ISR 등록
@param      void
@return     void
@remark
    - EPWM1 모듈을 UP-DOWN 카운터 모드로 설정합니다.
    - Period = 10,000 (200MHz 기준 100us 주기)
    - Zero Event 인터럽트 활성화 후 ISR 등록
    - EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1 (프리스케일러 1:1)
*/
void Initial_EpwmTimer(void)
{
    /* EPWM1 클럭 활성화 */
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM1);

    /* EPWM 설정 중 타이머가 멋대로 도는 것을 방지하기 위해 TBCLK 동기화 비활성화 */
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    /* EPWM1 리셋 후 초기화 */
    EPWM_setTimeBaseCounterMode(EPWM_TIMER1_BASE, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_setTimeBasePeriod(EPWM_TIMER1_BASE, (uint16_t)EPWM_TIMER1_PERIOD);
    EPWM_setTimeBaseCounter(EPWM_TIMER1_BASE, 0U);

    /* 프리스케일러 설정: CLKDIV=1, HSPCLKDIV=1 → TBCLK = SYSCLK = 200MHz */
    EPWM_setClockPrescaler(EPWM_TIMER1_BASE,
                           EPWM_TIMER1_CLK_DIV,
                           EPWM_TIMER1_HCLK_DIV);

    /* Zero Event 인터럽트 활성화 (카운터가 0이 될 때마다 인터럽트) */
    EPWM_setInterruptSource(EPWM_TIMER1_BASE, EPWM_INT_TBCTR_ZERO);
    EPWM_enableInterrupt(EPWM_TIMER1_BASE);
    EPWM_setInterruptEventCount(EPWM_TIMER1_BASE, 1U); /* 매 1회 이벤트마다 인터럽트 */

    /* 설정 완료 후 전체 EPWM TBCLK 동기화 재활성화 (모든 타이머 동시 카운트 시작) */
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
}


