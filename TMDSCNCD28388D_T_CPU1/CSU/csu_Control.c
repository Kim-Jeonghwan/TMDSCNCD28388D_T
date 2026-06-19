/**********************************************************************
 Nexcom Co., Ltd.
 Filename         : csu_Control.c
 Version          : 00.00
 Description      : 시스템 메인 제어 및 인터럽트 로직 구현
 Programmer       : Kim Jeonghwan
 Last Updated     : 2026. 06. 19. (EPWM 플래그 초기화 추가)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - Control_Init() 내부 EPWM_clearEventTriggerInterruptFlag 강제 클리어 구문 추가
 * 2026. 06. 19. - hal_Epwm.c에서 100us 타이머 ISR 및 제어 로직 이전
 */

#include "csu_Control.h"

#define SINE_WAVE_STEP 0.0062831853f // 100Hz = 100us * 1000 step
static float32_t sineAngle = 0.0f;
static uint32_t ipcSeqNum = 0U;

/*
@function   Control_Init
@brief      시스템 제어 인터럽트 초기화
@param      void
@return     void
@remark
    - EPWM1 인터럽트에 메인 제어 루틴(MainControl_Isr)을 등록합니다.
*/
void Control_Init(void)
{
    Interrupt_register(INT_EPWM1, MainControl_Isr);
    
    /* PIE 인터럽트를 켜기 전, 펜딩된 EPWM 플래그를 강제로 완전히 지워 Deadlock 방지 */
    EPWM_clearEventTriggerInterruptFlag(EPWM_TIMER1_BASE);
    
    Interrupt_enable(INT_EPWM1);
}

/*
@function   MainControl_Isr
@brief      시스템 메인 100us 제어 루프 (EPWM1 ISR)
@param      void
@return     __interrupt void
@remark
    - 100us 마다 호출됩니다.
    - ADC를 갱신하고 사인파(Sine wave)를 생성합니다.
    - 갱신된 데이터를 IPC의 Payload에 담아 CM으로 전송합니다.
*/
__interrupt void MainControl_Isr(void)
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

    /* GPIO34 임시 LED 500ms 토글 (육안 점멸 확인 목적) */
    static uint16_t ledToggleCnt = 0U;
    if (++ledToggleCnt >= 5000U) // 100us * 5000 = 500ms
    {
        GPIO_togglePin(34U);
        ledToggleCnt = 0U;
    }
}
