/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Control.c
    Version          : 00.02
    Description      : 시스템 메인 제어 및 인터럽트 로직 구현
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비 및 매크로 이동)
 **********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (매크로 상수 헤더 이동 및 이력 보완)
 * 2026. 06. 22. - GSRAM 잔재 주석을 MSGRAM 기준으로 수정
 * 2026. 06. 19. - Control_Init() 내부 EPWM_clearEventTriggerInterruptFlag 강제 클리어 구문 추가
 * 2026. 06. 19. - hal_Epwm.c에서 100us 타이머 ISR 및 제어 로직 이전
 * 2026. 06. 22. - IPC_sendCommand 대신 MSGRAM Seqlock 동기화 및 Try-Lock 적용
 * 2026. 06. 22. - 파형 선택 기능(Sine, Square, Triangle) 추가 및 waveType 기반 스위치문 적용
 */

#include "csu_Control.h"

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
    - ADC를 갱신하고 PC에서 수신한 파형 종류(waveType)에 따라 파형(Sine, Square, Triangle)을 생성합니다.
    - CM으로부터 CMTOCPU1 MSGRAM을 Try-Lock 방식으로 읽어 수신된 이더넷 패킷 데이터를 갱신합니다.
    - 갱신된 데이터를 CPU1TOCM MSGRAM Payload에 Seqlock을 이용해 CM으로 전달합니다.
*/
__interrupt void MainControl_Isr(void)
{
    /* 1. ADC 데이터 갱신 */
    updateAdcData();

    /* 2. 파형 생성 (0.5Hz 기준) */
    float32_t waveValue = 0.0f;
    switch(xEthRxData.waveType)
    {
        case 1U: /* Square Wave */
            waveValue = (sineAngle < 3.14159265f) ? 1.0f : -1.0f;
            break;
        case 2U: /* Triangle Wave */
            if (sineAngle < 3.14159265f)
            {
                waveValue = (sineAngle / 3.14159265f) * 2.0f - 1.0f;
            }
            else
            {
                waveValue = 1.0f - ((sineAngle - 3.14159265f) / 3.14159265f) * 2.0f;
            }
            break;
        case 0U: /* Sine Wave */
        default:
            waveValue = sinf(sineAngle);
            break;
    }

    sineAngle += SINE_WAVE_STEP;
    if (sineAngle > 6.2831853f) // 2 * PI
    {
        sineAngle -= 6.2831853f;
    }

    /* 3. CM이 CMTOCPU1 MSGRAM에 기록한 데이터 읽기 (Try-Lock 방식) */
    uint32_t seq0 = pxDataCmToCpu1->seqCount;
    // CM이 쓰는 중(홀수)이 아닐 때만 대기 없이 읽기 시도하여 실시간성(Jitter 0) 보장
    if ((seq0 & 1U) == 0U)
    {
        uint32_t tempSeq = pxDataCmToCpu1->Payload.RxData.seqNum;
        uint32_t tempWaveType = pxDataCmToCpu1->Payload.RxData.waveType;
        uint32_t seq1 = pxDataCmToCpu1->seqCount;
        
        if (seq0 == seq1)
        {
            // 정상적으로 읽힌 경우 전역 변수 갱신
            xEthRxData.seqNum = (uint8_t)(tempSeq & 0xFFU);
            xEthRxData.waveType = (uint8_t)(tempWaveType & 0xFFU);
        }
    }

    /* 4. CM으로 CPU1TOCM MSGRAM 데이터 쓰기 (Seqlock 방식) */
    pxDataCpu1ToCm->seqCount++; // 홀수로 변경 (쓰기 시작)
    pxDataCpu1ToCm->Payload.TxData.waveValue = waveValue;
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
