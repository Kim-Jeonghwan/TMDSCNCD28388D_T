/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main_cpu1.c
    Version          : 00.02
    Description      : CPU1 코어 메인 루프 및 태스크
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (전역 인터럽트 위치 CM 대기 루프 이전으로 교정)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - 데드락 해결: EINT, ERTM 위치를 g_bCmReady 대기 루프 이전으로 재이동 (IPC ISR 수신 필수)
 * 2026. 06. 19. - 전역 인터럽트(EINT, ERTM) 호출 위치를 안전한 while(1) 직전으로 대이동
 * 2026. 06. 19. - Control_Init() 호출 추가 (CSU 메인 제어 루프 등록)
 * 2026. 06. 19. - 코드 스타일 및 주석 템플릿 적용
 * 2026. 06. 05. - 코드 주석 포맷팅 및 한글화
 */

/* ************************** [[   include  ]]  *********************************************************** */
#include "main_cpu1.h"



/* ************************** [[   define   ]]  *********************************************************** */



/* ************************** [[   global   ]]  *********************************************************** */



/* ************************** [[  static prototype  ]]  *************************************************** */
// 1ms 주기(1000Hz)
static void cycle_1ms(void);

// 10ms(100Hz)
static void cycle_10ms(void);

// 100ms(10Hz)
static void cycle_100ms(void);

// 1000ms(1Hz)
static void cycle_1000ms(void);


/* ************************** [[  function  ]]  *********************************************************** */
/*
@funtion	void main(void)
@brief		CPU1 코어 메인 엔트리 포인트 및 백그라운드 태스크 스케줄러
@param		void
@return		void
@remark	
	- 시스템 초기화 및 CM 코어와의 동기화를 수행한 후, 주기에 맞춰 태스크를 실행합니다.
*/
void main(void)
{
	DSP_Initialization();

	/* --- [제어 루프 등록] 인터럽트 벡터 연결 --- */
	Control_Init();

	/* --- [핵심 개선] CM 코어와 IPC 하드웨어 동기화 (두 코어가 준비될 때까지 대기) --- */
	Initial_IPC();

	/* --- [정석 타이밍 적용] 모든 초기화와 인터럽트 벡터 등록이 완료된 시점에 전역 인터럽트 일제히 가동 --- */
	/* (주의: 아래의 CM 대기 루프 탈출을 위해서는 IPC 수신 인터럽트가 필수이므로 여기서 켜야 함) */
	ERTM;   // Debug Enable Mask 비트 설정 (실시간 디버깅 가능)
	EINT;   // 전역 인터럽트 스위치 ON (/INTM ON)

	/* --- [핵심 개선] CM 코어가 통신 및 주변장치 기동을 마칠 때까지 안전하게 대기 --- */
	while (xIpcState.isCmReady == false)
	{
		// CM 코어가 모든 부팅 및 이더넷/인터럽트 초기화를 끝내고 READY 신호를 쏠 때까지 대기
	}

	// 백그라운드 유휴 루프 (Background Loop)
	while(1u)
	{
		sendScia_SCI_PC();

		while(xTimer.Cycle_1ms >= 1u)
		{
			xTimer.Cycle_1ms -= 1u;
			cycle_1ms();
		}

		while(xTimer.Cycle_10ms >= 10u)
		{
			xTimer.Cycle_10ms -= 10u;
			cycle_10ms();
		}

		while(xTimer.Cycle_100ms >= 100u)
		{
			xTimer.Cycle_100ms -= 100u;
			cycle_100ms();
		}

		while(xTimer.Cycle_1000ms >= 1000u)
		{
			xTimer.Cycle_1000ms -= 1000u;
			cycle_1000ms();
		}
	}
}



/*
@funtion	static void cycle_1ms(void)
@brief		1ms 주기로 실행되는 주기 Task
@param		void
@return		static void
@remark	
	- 시스템 클럭 계수(Hzcnt)를 증가시키는 등 고속 모니터링 작업을 실시간 처리합니다.
*/
static void cycle_1ms(void)
{
	// 사용자 코드 작성
	xTimer.Hzcnt++;
}



/*
@funtion	static void cycle_10ms(void)
@brief		10ms 주기로 실행되는 주기 Task
@param		void
@return		static void
@remark	
	- ADC 센서 데이터를 갱신하고 PC로 보낼 통신 메시지를 처리합니다.
*/
static void cycle_10ms(void)
{
    // 3. 통신 메시지 송신
    sendSciPcMessage1();
}


/*
@funtion	static void cycle_100ms(void)
@brief		100ms 주기로 실행되는 주기 Task
@param		void
@return		static void
@remark	
	- 시스템 상태 모니터링 및 LED 상태를 갱신합니다.
*/
static void cycle_100ms(void)
{
    updateLedStatus();
}




/*
@funtion	static void cycle_1000ms(void)
@brief		1000ms(1초) 주기로 실행되는 주기 Task
@param		void
@return		static void
@remark	
	- 디바이스 상태 및 저속 이벤트 처리를 위한 예약 필드입니다.
*/
static void cycle_1000ms(void)
{


}
