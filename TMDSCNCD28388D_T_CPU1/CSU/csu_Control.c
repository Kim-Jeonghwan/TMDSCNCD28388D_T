/**********************************************************************
 Nexcom Co., Ltd.
 Filename         : csu_Control.c
 Version          : 00.01
 Description      : 시스템 메인 제어 및 인터럽트 로직 구현
 Programmer       : Kim Jeonghwan
 Last Updated     : 2026. 06. 22. (GSRAM 동기화 적용 및 IPC 제거)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - Control_Init() 내부 EPWM_clearEventTriggerInterruptFlag 강제 클리어 구문 추가
 * 2026. 06. 19. - hal_Epwm.c에서 100us 타이머 ISR 및 제어 로직 이전
 * 2026. 06. 22. - IPC_sendCommand 대신 GSRAM Seqlock 동기화 및 Try-Lock 적용
 */

#include "csu_Control.h"

#define SINE_WAVE_STEP 0.000314159f // 0.5Hz = 100us * 20000 step (PC 모니터링 시각화용)
static float32_t sineAngle = 0.0f;
static uint32_t ipcSeqNum = 0U;
static uint32_t isrTickCnt = 0U;

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
    - CM으로부터 GS1을 Try-Lock 방식으로 읽어 수신된 이더넷 패킷 데이터를 갱신합니다.
    - 갱신된 데이터를 GS0 Payload에 Seqlock을 이용해 CM으로 전달합니다.
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

    /* 3. CM이 GS1에 기록한 데이터 읽기 (Try-Lock 방식) */
    uint32_t seq0 = pxDataCmToCpu1->seqCount;
    // CM이 쓰는 중(홀수)이 아닐 때만 대기 없이 읽기 시도하여 실시간성(Jitter 0) 보장
    if ((seq0 & 1U) == 0U)
    {
        uint32_t tempSeq = pxDataCmToCpu1->Payload.RxData.seqNum;
        uint32_t tempStatus = pxDataCmToCpu1->Payload.RxData.status;
        uint32_t seq1 = pxDataCmToCpu1->seqCount;
        
        if (seq0 == seq1)
        {
            // 정상적으로 읽힌 경우 전역 변수 갱신
            xEthRxData.seqNum = (uint8_t)(tempSeq & 0xFFU);
            xEthRxData.status = (uint8_t)(tempStatus & 0xFFU);
        }
    }

    /* 4. CM으로 GS0 데이터 쓰기 (Seqlock 방식) */
    pxDataCpu1ToCm->seqCount++; // 홀수로 변경 (쓰기 시작)
    pxDataCpu1ToCm->Payload.TxData.sineValue = sineValue;
    pxDataCpu1ToCm->Payload.TxData.adcTemperature = xAdc.currentTemperatureC;
    pxDataCpu1ToCm->Payload.TxData.sequenceNum = ipcSeqNum;
    pxDataCpu1ToCm->seqCount++; // 짝수로 변경 (쓰기 완료)

    /* 시퀀스 넘버 100ms(1000틱) 주기 업데이트 */
    isrTickCnt++;
    if (isrTickCnt >= 1000U)
    {
        isrTickCnt = 0U;
        ipcSeqNum++;
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
