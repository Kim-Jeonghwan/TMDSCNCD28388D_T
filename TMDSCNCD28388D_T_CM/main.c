/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main.c
    Description      : CM Core Main Entry
    Last Updated     : 2026. 05. 29. (정적 시험 기준 준수 및 Task 용어 통일)
**********************************************************************/

#include "main.h"

// --- 정적 함수 선언 ---
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
    /* 1. 시스템 초기화 (CM 코어 클럭 및 인터럽트 등) */
    CM_init(); 

    /* 2. 통신 및 주변장치 초기화 */
    Initial_IPC();
    Initial_Ethernet();
    Initial_TIMER();
    
    /* 2.5 전역 인터럽트 활성화 */
    (void)Interrupt_enableInProcessor(); 

    /* 3. 백그라운드 무한 루프 (Background Loop) */
    while(1)
    {
        /* --- 1ms Task --- */
        if (xTimer.Cycle_1ms >= 1)
        {
            xTimer.Cycle_1ms = 0;
            Cycle_1ms();
        }

        /* --- 10ms Task --- */
        if (xTimer.Cycle_10ms >= 10)
        {
            xTimer.Cycle_10ms = 0;
            Cycle_10ms();
        }

        /* --- 100ms Task --- */
        if (xTimer.Cycle_100ms >= 100)
        {
            xTimer.Cycle_100ms = 0;
            Cycle_100ms();
        }

        /* --- 1000ms Task --- */
        if (xTimer.Cycle_1000ms >= 1000)
        {
            xTimer.Cycle_1000ms = 0;
            Cycle_1000ms();
        }

        // 이더넷 패킷 처리 (상시 백그라운드 폴링)
        updateEthernetTask();
    }
}

// --- 주기별 Task 구현부 ---

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
    // 1ms 작업 내용
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
    // 100ms 작업 내용
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
