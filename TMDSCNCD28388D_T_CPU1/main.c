/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main.c
    Description      : Main background loop and periodic tasks
    Last Updated     : 2026. 06. 01. (updateAdcData 호출 주석 해제 및 정리)
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

	// 백그라운드 유휴 루프 (Background Loop)
	while(1u)
	{
		if(xTimer.Cycle_1ms >= 1u)
		{
			cycle_1ms();
			xTimer.Cycle_1ms = 0u;
		}

		if(xTimer.Cycle_10ms >= 10u)
		{
			cycle_10ms();
			xTimer.Cycle_10ms = 0u;
		}

		if(xTimer.Cycle_100ms >= 100u)
		{
			cycle_100ms();
			xTimer.Cycle_100ms = 0u;
		}

		if(xTimer.Cycle_1000ms >= 1000u)
		{
			cycle_1000ms();
			xTimer.Cycle_1000ms = 0u;
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
