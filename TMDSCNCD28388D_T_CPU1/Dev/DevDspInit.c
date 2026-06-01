/**********************************************************************

	Nexcom Co., Ltd.
	Copyright 2021. All Rights Reserved.

	Filename		: DevDspInit.c
	Version			: 00.00
	Description		: 
	Tracebility		: 
	Programmer	    :
	Last Updated	: 2026. 05. 29. (복잡도 최적화 및 ADC/PWM 분리 리팩토링)

**********************************************************************/

/*
 * Modification History
 * --------------------
 * 
 * 
*/


/* ************************** [[   include  ]]  *********************************************************** */
#include "DevDspInit.h"

/* ************************** [[   define   ]]  *********************************************************** */


/* ************************** [[   global   ]]  *********************************************************** */


/* ************************** [[  static prototype  ]]  *************************************************** */
static void Initial_GPIO(void);

static void Init_GpioDin(void);

static void Init_GpioDout(void);

static void InitialPeripherals(void);
static void Initial_CmCore(void);

// Helper functions for peripheral initialization (Complexity reduction)
static void initSystemAnalogAdc(void);
static void initSystemPwm(void);
static void initSystemUserInterface(void);
static void initSystemCommunications(void);


/* ************************** [[  function  ]]  *********************************************************** */
/*
@funtion	void DSP_Initialization(void)
@brief		DSP 초기화 
@param		void
@return		void
@remark	
	-	
*/
void DSP_Initialization(void)
{
	// 시스템 및 주변회로 클럭 설정
	Device_init();

    // 1. CM 코어가 사용할 Shared RAM 권한을 먼저 부여 (CM의 .data, .bss 초기화에 필수)
    Initial_IPC_Mastership();

    // 2. CM 코어 부팅 (Reset 해제)
    Initial_CmCore();

	Initial_GPIO();

	// 주변회로 인터럽트 확장 회로(PIE) 및 관련 레지스터 초기화 / CPU 인터럽트 비-활성화
	Interrupt_initModule();

	// PIE 벡터 테이블 초기화 및 기본 인터럽트 서비스 루틴 연결
	Interrupt_initVectorTable();


	InitialPeripherals();

	// 실시간 디버깅 활성화, 전역 인터럽트 스위치 ON
	ERTM;	// Debug Enable Mask 비트 설정 (실시간 디버깅이 가능하도록 ST1 레지스터의 /DBGM 비트를 0으로 클리어)
	EINT;	// 전역 인터럽트 스위치 ON (/INTM ON)
}






/*
@funtion	static void Initial_GPIO(void)
@brief		GPIO 초기화(DIN or DOUT)
@param		void
@return		static void
@remark	
	-	CAN, SPI, SCI, I2C GPIO는 각각  설정함 
*/

static void Initial_GPIO(void)
{
	Init_GpioDin();
	
	Init_GpioDout();
}

/*
@funtion	static void Init_GpioDin(void)
@brief		
@param		void
@return		static void
@remark	
	-	
*/
static void Init_GpioDin(void)
{
    // GPIO 1: 입력 설정 (GND 체크용)
    GPIO_setPinConfig(GPIO_1_GPIO1);
    GPIO_setPadConfig(1, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(1, GPIO_DIR_MODE_IN);
}



/*
@funtion	static void Init_GpioDout(void)
@brief		
@param		void
@return		static void
@remark	
	-	
*/
static void Init_GpioDout(void)
{
	initGpioDoutLed();

    // GPIO 31: 출력 설정 (CM 제어 테스트용)
    GPIO_setPinConfig(GPIO_31_GPIO31);
    GPIO_setPadConfig(31, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(31, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(31, GPIO_CORE_CPU1); // 초기 권한은 CPU1
}



/*
@funtion	static void DevInitPeripherals(void)
@brief		DSP 주변 디바이스 초기화 설정
@param		void
@return		static void
@remark	
	-	
*/
static void InitialPeripherals(void)
{
	initSystemAnalogAdc();
	initSystemPwm();
	initSystemUserInterface();
	initSystemCommunications();
}

/*
@funtion    static void initSystemAnalogAdc(void)
@brief      ADC 등 아날로그 입력 하드웨어 초기화 묶음
@param      void
@return     static void
*/
static void initSystemAnalogAdc(void)
{
	InitialAdc();
	Initial_Adc();
}

/*
@funtion    static void initSystemPwm(void)
@brief      EPWM 등 펄스 폭 변조 제어 하드웨어 초기화 묶음
@param      void
@return     static void
*/
static void initSystemPwm(void)
{
	initEPWM8();
	Initial_Epwm7a();
}

/*
@funtion    static void initSystemUserInterface(void)
@brief      LED, Tact, EQEP 등 사용자 인터페이스 및 입력장치 초기화 묶음
@param      void
@return     static void
*/
static void initSystemUserInterface(void)
{
	Initial_LED();
}

/*
@funtion    static void initSystemCommunications(void)
@brief      SPI, SCI, Timer, CAN, IPC 등 통신 및 코어 연동 초기화 묶음
@param      void
@return     static void
*/
static void initSystemCommunications(void)
{
	Initial_SPI();
	Initial_SCI();
	Initial_TIMER();
	Initial_IPC();
}

/*
@funtion	static void Initial_CmCore(void)
@brief		CM 코어 부팅 및 클럭 설정
@param		void
@return		static void
*/
static void Initial_CmCore(void)
{
    // CM 클럭 활성화 (안전한 플래시 쓰기를 위해 100MHz로 설정)
    SysCtl_setCMClk(SYSCTL_CMCLKOUT_DIV_2, SYSCTL_SOURCE_SYSPLL);

#ifdef _FLASH
    Device_bootCM(BOOTMODE_BOOT_TO_FLASH_SECTOR0);
#else
    Device_bootCM(BOOTMODE_BOOT_TO_S0RAM);
#endif
}
