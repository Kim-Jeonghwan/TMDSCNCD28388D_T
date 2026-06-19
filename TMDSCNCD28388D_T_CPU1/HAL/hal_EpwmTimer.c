/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_EpwmTimer.c
    Version          : 00.02
    Description      : CPU1 EPWM1 기반 100us 메인 인터럽트 구현
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (ISR 내 GPIO 34 토글 추가)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - isr_Epwm1Timer100us 내부에 GPIO 34 토글 로직 추가 (ATTLA_T 동기화)
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
 *   xXmtSciPcMsg1.DspTemp : csu_SCI_PC.c 에서 ADC 읽어서 갱신 (x10 스케일)
 *   xXmtSciPcMsg1.IncNumber: 시퀀스 번호
 *   xXmtSciPcMsg1.Status   : 상태 바이트
 */

#include "hal_EpwmTimer.h"

/* ISR 정적 선언 */
static __interrupt void isr_Epwm1Timer100us(void);

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
    Interrupt_register(INT_EPWM1, isr_Epwm1Timer100us);
    Interrupt_enable(INT_EPWM1);
}

/*
@function   isr_Epwm1Timer100us
@brief      EPWM1 100us 타이머 ISR (메인 루프)
@param      void
@return     static __interrupt void
@remark
    - 100us 마다 호출됩니다.
    - ADC를 갱신하고 사인파(Sine wave)를 생성합니다.
    - 갱신된 데이터를 IPC의 Payload에 담아 CM으로 전송합니다.
*/
#define SINE_WAVE_STEP 0.0062831853f // 100Hz = 100us * 1000 step
static float32_t sineAngle = 0.0f;
static uint32_t ipcSeqNum = 0U;

static __interrupt void isr_Epwm1Timer100us(void)
{
    /* 1. ADC 데이터 갱신 */
    updateAdcData();

    /* 2. 사인파 생성 (100Hz 기준) */
    float32_t sineValue = sinf(sineAngle);
    sineAngle += SINE_WAVE_STEP;
    if (sineAngle > 6.2831853f) // 2 * PI
    {
        sineAngle -= 6.2831853f;
    }

    /* 3. CM으로 IPC 데이터 전송 (TxData 캡슐화) */
    if (!IPC_isFlagBusyLtoR(IPC_CPU1_L_CM_R, IPC_FLAG1))
    {
        pxIpcCpu1ToCm->Payload.TxData.sineValue = sineValue;
        pxIpcCpu1ToCm->Payload.TxData.adcTemperature = xAdc.currentTemperatureC;
        pxIpcCpu1ToCm->Payload.TxData.sequenceNum = ipcSeqNum++;
        
        IPC_sendCommand(IPC_CPU1_L_CM_R, IPC_FLAG1, IPC_ADDR_CORRECTION_DISABLE, 
                        (uint32_t)IPC_CMD_CPU1_ETH_TX_DATA, 0U, 0U);
    }

    /* EPWM 인터럽트 플래그 클리어 */
    EPWM_clearEventTriggerInterruptFlag(EPWM_TIMER1_BASE);

    /* PIE ACK */
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP3);

    /* GPIO34 임시 LED 100us 토글 (ATTLA_T 동기화) */
    GPIO_togglePin(34U);
}
