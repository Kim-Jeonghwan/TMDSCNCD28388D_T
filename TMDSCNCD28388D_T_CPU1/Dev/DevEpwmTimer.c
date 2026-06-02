/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevEpwmTimer.c
    Description      : EPWM1 기반 2ms 하드웨어 타이머 구현 (UDP 이더넷 TX 트리거용)
    Last Updated     : 2026. 06. 01. (신규 작성)
**********************************************************************/

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
 *   xXmtSciPcMsg1.DspTemp : CSU_SCI_PC.c 에서 ADC 읽어서 갱신 (x10 스케일)
 *   xXmtSciPcMsg1.IncNumber: 시퀀스 번호
 *   xXmtSciPcMsg1.Status   : 상태 바이트
 */

#include "DevEpwmTimer.h"
#include "DevIPC.h"
#include "CSU_SCI_PC.h"

/* ISR 정적 선언 */
static __interrupt void isr_Epwm1Timer2ms(void);

/*
@funtion    void Initial_EpwmTimer(void)
@brief      EPWM1 기반 2ms 타이머 초기화 및 ISR 등록
@param      void
@return     void
@remark
    - EPWM1 모듈을 UP-DOWN 카운터 모드로 설정합니다.
    - Period = 200,000 (200MHz 기준 2ms 주기)
    - Zero Event 인터럽트 활성화 후 ISR 등록
    - EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1 (프리스케일러 1:1)
*/
void Initial_EpwmTimer(void)
{
    /* EPWM1 클럭 활성화 */
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM1);

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

    /* PIE 인터럽트 등록 및 활성화 */
    Interrupt_register(INT_EPWM1, isr_Epwm1Timer2ms);
    Interrupt_enable(INT_EPWM1);
}

/*
@funtion    static __interrupt void isr_Epwm1Timer2ms(void)
@brief      EPWM1 타이머 2ms Zero Event ISR - UDP 이더넷 TX 데이터 CM 전송
@param      void
@return     static __interrupt void
@remark
    - 2ms 마다 호출됩니다.
    - xXmtSciPcMsg1 (CSU_SCI_PC.h) 에서 온도/시퀀스 데이터를 읽어 CM으로 IPC 전송합니다.
    - CM은 수신 즉시 UDP Reflect 패킷을 PC로 송신합니다.
*/
static __interrupt void isr_Epwm1Timer2ms(void)
{
    /* CM 코어에 온도 + 시퀀스 + 상태 데이터 IPC 전송 */
    sendEthDataToCM(xXmtSciPcMsg1.DspTemp,
                    (uint8_t)xXmtSciPcMsg1.IncNumber,
                    (uint8_t)xXmtSciPcMsg1.Status);

    /* EPWM 인터럽트 플래그 클리어 */
    EPWM_clearEventTriggerInterruptFlag(EPWM_TIMER1_BASE);

    /* PIE ACK */
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP3);
}
