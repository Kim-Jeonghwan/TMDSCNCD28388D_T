/**********************************************************************

	Nexcom Co., Ltd.
	Copyright 2021. All Rights Reserved.

	Filename		: DevSpi.c
	Version			: 00.00
	Description		: 
	Tracebility		: 
	Programmer		: 
	Last Updated	: 2026. 05. 29. (정적 시험 기준 준수 및 함수 주석 보강)

	Function List	:	
						

**********************************************************************/

/*
 * Modification History
 * --------------------
 * 
 * 
 */


/* DESCRIPTION
 * 
 * 
 */


/* ************************** [[   include  ]]  *********************************************************** */
#include "DevSpi.h"

/* ************************** [[   define   ]]  *********************************************************** */
// #define SSI_SIMO_SPIB	63u // SPI SIMOB
#define ENCODER_SOMI_GPIC	51u // SPI SOMIC
#define ENCODER_CLK_GPIC	52u // SPI CLKC
// #define SSI_CS			66u // Chip Select

#define EEPROM_SIMO_GPIOA    58u // SPIA SIMO
#define EEPROM_SOMI_GPIOA    59u // SPIA SOMI
#define EEPROM_CLK_GPIOA     60u // SPIA CLK
#define EEPROM_CS_GPIOA      61u // SPIA CS (GPIO)

/* ************************** [[   global   ]]  *********************************************************** */


/* ************************** [[   static prototype  ]]  ************************************************** */
static void InitSpia(void);
static void InitSpic(void);



/* ************************** [[  function  ]]  *********************************************************** */
/*
@funtion	void Initial_SPI(void)
@brief		SPI 초기화 
@param		void
@return		void
@remark	
	-	ProtoType 은 DSP2833x_GlobalPrototypes.h 정의
	-	DevDspInit.c의 DevInitPeripherals() 호출 
*/
void Initial_SPI(void)
{
	// EEPROM
    InitSpia();

	// SSI
    InitSpic();

}



/*
@funtion	static void InitSpib(void)
@brief		
@param		void
@return		static void
@remark	
	-	
*/


/*
@funtion    static void InitSpia(void)
@brief      EEPROM 통신용 SPI-A 모듈 초기화 및 GPIO 설정
@param      void
@return     static void
@remark
    - GPIO 58, 59, 60번을 SPI-A 기능으로 할당하고 CS핀(GPIO 61)을 출력으로 제어합니다.
    - SPI-A 모듈을 Master 모드, 1MHz, 8비트 단위 통신 규격으로 구동합니다.
*/
static void InitSpia(void)
{
    EALLOW; 

    // 핀 설정
    GPIO_setPinConfig(GPIO_58_SPIA_SIMO);
    GPIO_setPadConfig(EEPROM_SIMO_GPIOA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(EEPROM_SIMO_GPIOA, GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_59_SPIA_SOMI);
    GPIO_setPadConfig(EEPROM_SOMI_GPIOA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(EEPROM_SOMI_GPIOA, GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_60_SPIA_CLK);
    GPIO_setPadConfig(EEPROM_CLK_GPIOA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(EEPROM_CLK_GPIOA, GPIO_QUAL_ASYNC);

    // CS를 일반 GPIO 출력으로 설정
    GPIO_setPinConfig(GPIO_61_GPIO61);
    GPIO_setPadConfig(EEPROM_CS_GPIOA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(EEPROM_CS_GPIOA, GPIO_QUAL_ASYNC);
    GPIO_setDirectionMode(EEPROM_CS_GPIOA, GPIO_DIR_MODE_OUT);
    GPIO_writePin(EEPROM_CS_GPIOA, 1); // CS 초기 상태 High (비활성)

    // SPI 초기화
    SPI_disableModule(SPIA_BASE);
    SPI_setConfig(SPIA_BASE, 
					DEVICE_LSPCLK_FREQ, 
					SPI_PROT_POL0PHA1, // C2000의 POL0PHA1 이 표준 SPI Mode 0 (Data latched on rising edge) 와 일치함
					SPI_MODE_MASTER, 
					1000000u,          // 1MHz
					8);                // EEPROM 통신은 8비트 단위
    SPI_disableFIFO(SPIA_BASE);
    SPI_setEmulationMode(SPIA_BASE, SPI_EMULATION_STOP_AFTER_TRANSMIT);
    SPI_enableModule(SPIA_BASE);

    EDIS;
}

/*
@funtion    static void InitSpic(void)
@brief      SSI 엔코더 통신용 SPI-C 모듈 초기화 및 GPIO 설정
@param      void
@return     static void
@remark
    - GPIO 51(SOMI), 52(CLK)를 SPI-C 기능으로 할당합니다.
    - Master 모드, Mode 2(POL1PHA0), 1MHz 속도 및 16비트 워드 규격으로 초기화합니다.
*/
static void InitSpic(void)
{
    EALLOW; // 보호 레지스터 쓰기 허용

    // 핀 설정
    // GPIO_setPinConfig(GPIO_63_SPIB_SIMO);
    // GPIO_setPadConfig(SSI_SIMO_SPIB, GPIO_PIN_TYPE_STD);
    // GPIO_setQualificationMode(SSI_SIMO_SPIB, GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_51_SPIC_SOMI);
    GPIO_setPadConfig(ENCODER_SOMI_GPIC, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(ENCODER_SOMI_GPIC, GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_52_SPIC_CLK);
    GPIO_setPadConfig(ENCODER_CLK_GPIC, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(ENCODER_CLK_GPIC, GPIO_QUAL_ASYNC);

    // GPIO_setPinConfig(GPIO_66_GPIO66);
    // GPIO_setPadConfig(SSI_CS, GPIO_PIN_TYPE_STD);
    // GPIO_setQualificationMode(SSI_CS, GPIO_QUAL_SYNC);
    // GPIO_setDirectionMode(SSI_CS, GPIO_DIR_MODE_OUT);
    // GPIO_setMasterCore(SSI_CS, GPIO_CORE_CPU1);



	// SPI 초기화. 1MHz SPICLK, Mode-2(POL1PHA0), 16비트 워드 크기 설정.
    SPI_disableModule(SPIC_BASE);
    SPI_setConfig(SPIC_BASE, 
					DEVICE_LSPCLK_FREQ, 
					SPI_PROT_POL1PHA0,                                          // SSI엔코더는 보통 클럭이 High로 대기하다가 첫 번째 하강 엣지에서 데이터를 내보내는 Mode 2 나 Mode 3 많이 씀 (현재 모드2)
					SPI_MODE_MASTER, 
					1000000u,                                                   // 일단 1MHz(260126) - 필요 시 10MHz로 변경
					16);                                                        // 8 비트 : 8, 16 비트 : 16
    SPI_disableFIFO(SPIC_BASE);
    SPI_setEmulationMode(SPIC_BASE, SPI_EMULATION_STOP_AFTER_TRANSMIT);
    SPI_enableModule(SPIC_BASE);

    EDIS;   // 보호 레지스터 쓰기 금지
}

