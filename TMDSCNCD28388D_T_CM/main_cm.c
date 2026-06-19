/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main_cm.c
    Version          : 00.02
    Description      : CM 코어 메인 루프 및 태스크
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (Cycle_100ms 내부에 GPIO 145 토글 로직 추가)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - Cycle_100ms 내부에 GPIO 145 토글 로직 추가 (500ms 주기)
 * 2026. 06. 19. - 코드 스타일 및 주석 템플릿 적용
 * 2026. 06. 05. - 코드 주석 포맷팅 및 한글화
 */

#include "main_cm.h"

// --- 정적 함수 선언 ---
static void Cycle_2ms(void);
static void Cycle_1ms(void);
static void Cycle_10ms(void);
static void Cycle_100ms(void);
static void Cycle_1000ms(void);

/*
@funtion    int main(void)
@brief      CM 코어 메인 엔트리 포인트 및 백그라운드 태스크 스케줄러
@param      void
@return     int
@remark
    - 시스템 및 통신 디바이스(IPC, Ethernet, Timer)를 기동하고 백그라운드 라운드 로빈 방식으로 주기별 태스크를 조율합니다.
*/
int main(void)
{
    /* --- [핵심 개선] CM 코어 기본 하드웨어 및 클럭 초기화 --- */
    CM_init(); 
    
    /* --- [핵심 개선] 인터럽트 벡터 테이블을 RAM으로 복사 및 활성화 --- */
    /* 반드시 Initial_IPC() 등의 인터럽트 등록 함수보다 '먼저' 호출되어야 합니다! */
#ifdef _FLASH
    Interrupt_initRAMVectorTable(vectorTableFlash, vectorTableRAM);
#endif

    /* 2. 통신 및 주변장치 초기화 및 동기화 */
    Initial_IPC();       // CPU1과 안전하게 1단계 하드웨어 동기화 (대기 탈출)

    /* --- [물리 이더넷 프리징 예방] PHY 칩 리셋 해제 후 하드웨어 안정화 대기 딜레이 (100ms) --- */
    /* 동기화를 성공적으로 탈출한 직후, PHY가 기상하는 시간 동안 안전하게 대기합니다. */
    {
        volatile uint32_t uiDelay = 0U;
        for(uiDelay = 0U; uiDelay < 12000000U; uiDelay++)
        {
            __asm(" NOP");
        }
    }

    Initial_Ethernet();  // 딜레이 탈출 즉시 이더넷 및 타이머 기동
    Initial_TIMER();
    
    /* 2.5 전역 인터럽트 활성화 */
    (void)Interrupt_enableInProcessor(); 

    /* --- [핵심 개선] CM 코어의 모든 초기화 완료 및 기동 상태를 CPU1로 최종 통보 (2단계 핸드셰이크) --- */
    sendIpcMessageToCPU1(IPC_CMD_CM_BOOT_READY, 0U, 0U);

    /* 3. 백그라운드 무한 루프 (Background Loop) */
    while(1)
    {
        /* --- 2ms Task: UDP Reflect MSG 송신 --- */
        while (xTimer.Cycle_2ms >= 1U)
        {
            xTimer.Cycle_2ms -= 1U;
            Cycle_2ms();
        }

        /* --- 1ms Task --- */
        while (xTimer.Cycle_1ms >= 1U)
        {
            xTimer.Cycle_1ms -= 1U;
            Cycle_1ms();
        }

        /* --- 10ms Task --- */
        while (xTimer.Cycle_10ms >= 10u)
        {
            xTimer.Cycle_10ms -= 10u;
            Cycle_10ms();
        }

        /* --- 100ms Task --- */
        while (xTimer.Cycle_100ms >= 100u)
        {
            xTimer.Cycle_100ms -= 100u;
            Cycle_100ms();
        }

        /* --- 1000ms Task --- */
        while (xTimer.Cycle_1000ms >= 1000u)
        {
            xTimer.Cycle_1000ms -= 1000u;
            Cycle_1000ms();
        }
    }
}

// --- 주기별 Task 구현부 ---

/*
@funtion    static void Cycle_2ms(void)
@brief      2ms 주기 UDP Reflect MSG 송신 Task
@param      void
@return     static void
@remark
    - CPU1이 IPC로 전달한 온도/시퀀스 데이터를 UDP 패킷으로 조립하여 PC로 송신합니다.
*/
static void Cycle_2ms(void)
{
    // PC에서 데이터 요청 패킷 수신 시 응답하므로, 2ms 주기 자가 전송은 제거합니다.
}

/*
@funtion    static void Cycle_1ms(void)
@brief      1ms 주기로 실행되는 주기 Task
@param      void
@return     static void
@remark
    - 시스템 클럭 계수(Hzcnt)를 증가시키는 등 고속 모니터링 작업을 실시간 처리합니다.
*/
static void Cycle_1ms(void)
{
    xTimer.Hzcnt++;
}

/*
@funtion    static void Cycle_10ms(void)
@brief      10ms 주기로 실행되는 주기 Task
@param      void
@return     static void
@remark
    - 중속 통신 수신이나 센서 데이터 동기화 처리를 위한 예약 필드입니다.
*/
static void Cycle_10ms(void)
{
    // 10ms 작업 내용
}

/*
@funtion    static void Cycle_100ms(void)
@brief      100ms 주기로 실행되는 주기 Task
@param      void
@return     static void
@remark
    - 시스템 보드 LED 제어 및 상태 모니터링 처리를 수행하기 위한 예약 필드입니다.
*/
static void Cycle_100ms(void)
{
    // 500ms(100ms * 5) 주기로 점멸시키기 위한 로직
    static uint16_t cmLedCnt = 0U;
    if (++cmLedCnt >= 5U)
    {
        GPIO_togglePin(145U); // GPIO 145 토글 (CM 코어 API 사용)
        cmLedCnt = 0U;
    }
}

/*
@funtion    static void Cycle_1000ms(void)
@brief      1000ms(1초) 주기로 실행되는 주기 Task
@param      void
@return     static void
@remark
    - 저속 진단 장치 보고 및 디바이스 에러 자가진단을 위한 예약 필드입니다.
*/
static void Cycle_1000ms(void)
{
    // 1000ms 작업 내용
}
