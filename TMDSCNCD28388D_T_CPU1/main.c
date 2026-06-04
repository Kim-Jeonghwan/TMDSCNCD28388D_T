/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main.c
    Description      : Main background loop and periodic tasks
    Last Updated     : 2026. 06. 04. (스케줄러 while 루프 교정 및 Hzcnt 750Hz 편차 해결)
**********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"



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
@brief		
@param		void
@return		void
@remark	
	-	
*/
void main(void)
{
	DSP_Initialization();

	/* --- [핵심 개선] CM 코어와 IPC 하드웨어 동기화 (두 코어가 준비될 때까지 대기) --- */
	Initial_IPC();

	/* --- [핵심 개선] CM 코어가 통신 및 주변장치 기동을 마칠 때까지 안전하게 대기 --- */
	while (g_bCmReady == false)
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
@brief		1ms 마다 수행 하는 동작 
@param		void
@return		static void
@remark	
	-	
*/
static void cycle_1ms(void)
{
	// 사용자 코드 작성
	xTimer.Hzcnt++;
}



/*
@funtion	static void cycle_10ms(void)
@brief		10ms 마다 수행 하는 동작 
@param		void
@return		static void
@remark	
	-	
*/
static void cycle_10ms(void)
{
    updateAdcData();
    
    // 3. 통신 메시지 송신
    sendSciPcMessage1();
}


/*
@funtion	static void cycle_100ms(void)
@brief		100ms 마다 수행 하는 동작 
@param		void
@return		static void
@remark	
	-	
*/
static void cycle_100ms(void)
{
    updateLedStatus();
}




/*
@funtion	static void cycle_1000ms(void)
@brief		1000ms 마다 수행 하는 동작 
@param		void
@return		static void
@remark	
	-	
*/
static void cycle_1000ms(void)
{


}
