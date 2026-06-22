/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Spi.c
    Version          : 00.00
    Description      : SPI Driver for SSI Encoder
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비 및 매크로 이동)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (작성자 기입, 매크로 상수 이동 및 이력 보완)
 */


/* DESCRIPTION
 * 
 * 
 */


/* ************************** [[   include  ]]  *********************************************************** */
#include "hal_Spi.h"

/* ************************** [[   define   ]]  *********************************************************** */



/* ************************** [[   global   ]]  *********************************************************** */


/* ************************** [[   static prototype  ]]  ************************************************** */

static void InitSpic(void);



/* ************************** [[  function  ]]  *********************************************************** */
/*
@funtion    void Initial_SPI(void)
@brief      SPI 드라이버 초기화
@param      void
@return     void
@remark 
    - SSI 엔코더용 SPI-C 모듈 초기화를 호출합니다.
*/
void Initial_SPI(void)
{

	// SSI
    InitSpic();

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

