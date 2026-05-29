/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main.c
    Description      : 
    Last Updated     : 2026. 05. 29. (정적 시험 기준 준수 및 1ms 중복 호출 제거)
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

// 10ms 주기를 위한 헬퍼 함수 (복잡도 최적화)
static void processInputDevices(void);
static void processCanRoutine(void);



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
	processInputDevices();

    updateAdcData();
    
    // EPWM7A 상태 업데이트 (10ms 주기)
    updateEpwm7aStatus();

    // 3. 통신 메시지 송신
    sendSciPcMessage1();

    // 이더넷 루프백 테스트 업데이트
    updateLoopbackTest();

	processCanRoutine();

	updateEepromStatus();

}



/*
@funtion	static void processInputDevices(void)
@brief		10ms 주기 스위치 및 엔코더 입력 갱신
@param		void
@return		static void
*/
static void processInputDevices(void)
{
	EqeptoEncoder();
	updateHwSwitchStatus2();
	updateTactStatus();
}



/*
@funtion	static void processCanRoutine(void)
@brief		10ms 주기 CAN 통신 송수신 및 상태 관리
@param		void
@return		static void
*/
static void processCanRoutine(void)
{
	recvCanMessage();
	updateCanXmtData();
	sendCanMessage();
	updateCanStatus();
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
