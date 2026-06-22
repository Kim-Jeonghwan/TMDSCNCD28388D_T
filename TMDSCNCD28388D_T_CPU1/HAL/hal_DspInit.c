/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_DspInit.c
    Version          : 00.06
    Description      : CPU1 Master Initialization (CM Core Fault 해결을 위한 권한 양도 시퀀스 개편)
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비 및 인클루드 수정)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (자기 자신 헤더 인클루드 룰 정비 및 이력 보완)
 * 2026. 06. 22. - MSGRAM 롤백으로 인해 불필요해진 Initial_IPC_Mastership 호출 완전 제거
 * 2026. 06. 19. - 전역 인터럽트(EINT, ERTM) 호출을 main_cpu1.c 내부 while(1) 직전으로 이동 (안전성 확보)
 * 2026. 06. 19. - Device_init() 직후 전체 EPWM 클럭(EPWMCLK)을 200MHz(1:1 분주)로 교정하는 로직 추가
 * 2026. 06. 19. - Init_GpioDout 내부에 GPIO 145 제어권 CM 코어로 양도
 * 2026. 06. 19. - csu_Led.c에서 분리된 GPIO 31 및 34 초기화 로직을 Init_GpioDout()에 직접 통합
 * 2026. 06. 02. - CM 코어 기동 시점(Initial_CmCore)을 동기화(IPC_sync) 직전 최고의 타이밍으로 대이동 교정
 * 2026. 06. 02. - 온도 센서 전용 1kHz 느린 트리거용 ePWM9 모듈 추가 기동 반영
 * 2026. 06. 02. - 이더넷 PHY 칩(DP83822) 하드웨어 리셋 핀(GPIO 147) 강제 해제 추가
 * 2026. 06. 02. - C2000Ware 공식 예제 기준 이더넷 클럭 공급(setEnetClk) 및 GPIO108 PWDN 해제 설정 추가
 * 2026. 06. 02. - IPC_sync 대기 전에 이더넷 PHY 기동(initEmacGpioPins)이 되도록 기동 시퀀스 순서 최우선 개편
 * 2026. 06. 02. - 실험 A: GPIO108을 일반 GPIO 입력/풀업으로 원복하여 PHY 자체 스트랩 설정 보호
 * 2026. 06. 02. - 정석 Active-Low 리셋 시퀀스 복구 및 GPIO119 강력한 푸시풀 출력(STD) 모드 융합 적용
 * 2026. 06. 04. - IPC 동기화(Initial_IPC) 호출을 DSP_Initialization 내부에서 main.c로 상향 이동
 * 2026. 06. 04. - CM 하드폴트 원천 박멸을 위해 미존재 인터럽트 권한 양도 API 삭제 및 초기화 안정화
 */


/* ************************** [[    include    ]]    *********************************************************** */
#include "hal_DspInit.h"


/* ************************** [[    define     ]]    *********************************************************** */


/* ************************** [[    global     ]]    *********************************************************** */


/* ************************** [[    static prototype    ]]    *************************************************** */
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
static void initEmacGpioPins(void);  /* EMAC MII 핀 MUX 설정 */


/* ************************** [[    function    ]]    *********************************************************** */
/*
@funtion    void DSP_Initialization(void)
@brief      DSP 초기화 수행의 진입점
@param      void
@return     void
@remark 
    - CM 코어와 IPC 하드웨어를 동기화시키고 각종 페리페럴을 초기화합니다.
*/
void DSP_Initialization(void)
{
    // 시스템 및 주변회로 클럭 설정
    Device_init();

    /* --- [클럭 교정] EPWM 모듈의 입력 클럭(EPWMCLK)을 SYSCLK(200MHz)과 동일하게 1:1 분주로 명시적 설정 --- */
    SysCtl_setEPWMClockDivider(SYSCTL_EPWMCLK_DIV_1);

    /* --- [최우선 조치] 동기화(IPC_sync) 대기 전에 물리 이더넷 PHY부터 즉시 깨움 --- */
    initEmacGpioPins();


    Initial_GPIO();

    // 주변회로 인터럽트 확장 회로(PIE) 및 관련 레지스터 초기화 / CPU 인터럽트 비-활성화
    Interrupt_initModule();

    // PIE 벡터 테이블 초기화 및 기본 인터럽트 서비스 루틴 연결
    Interrupt_initVectorTable();

    /* --- [핵심 개선] CM 코어 기동 전에 IPC 레지스터 청소 완료 --- */
    Initial_IPC_Clear();

    // 주변 장치 하드웨어 초기화 미리 수행 (타이머 및 기타 통신망 셋업)
    InitialPeripherals();

    /* --- [정석 타이밍 적용] 모든 권한 양도와 하드웨어 준비가 100% 완료된 바로 이 시점에 CM 코어 기동 --- */
    Initial_CmCore();

    // 전역 인터럽트 활성화(EINT, ERTM)는 모든 시스템 초기화 및 인터럽트 등록이 완료된 
    // main_cpu1.c 의 while(1) 메인 루프 진입 직전에 수행하도록 이동되었습니다.
}

/*
@funtion    static void Initial_GPIO(void)
@brief      GPIO 초기화 (DIN / DOUT)
@param      void
@return     static void
@remark 
    - GPIO 입력 및 출력을 개별 단위로 나누어 초기화합니다.
*/
static void Initial_GPIO(void)
{
    Init_GpioDin();
    Init_GpioDout();
}

/*
@funtion    static void Init_GpioDin(void)
@brief      디지털 입력 GPIO 설정
@param      void
@return     static void
@remark 
    - 특정 핀을 풀업(Pull-up) 입력 모드로 구성합니다.
*/
static void Init_GpioDin(void)
{
    // GPIO 1: 입력 설정 (GND 체크용)
    GPIO_setPinConfig(GPIO_1_GPIO1);
    GPIO_setPadConfig(1u, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(1u, GPIO_DIR_MODE_IN);
}

/*
@funtion    static void Init_GpioDout(void)
@brief      디지털 출력 GPIO 설정
@param      void
@return     static void
@remark 
    - 보드 내 상태 표시용 LED 등을 제어하기 위한 출력 핀을 초기화합니다.
*/
static void Init_GpioDout(void)
{
    // GPIO 31: RUN LED 출력 설정
    GPIO_setPinConfig(GPIO_31_GPIO31);
    GPIO_setPadConfig(31u, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(31u, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(31u, GPIO_CORE_CPU1); // 초기 권한은 CPU1

    // GPIO 34: 메인 컨트롤 ISR 동작 확인용 임시 LED
    GPIO_setPinConfig(GPIO_34_GPIO34);
    GPIO_setPadConfig(34u, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(34u, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(34u, GPIO_CORE_CPU1);

    // GPIO 145: CM 코어 구동 상태 확인용 LED
    GPIO_setPinConfig(GPIO_145_GPIO145);
    GPIO_setPadConfig(145u, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(145u, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(145u, GPIO_CORE_CM); // CM으로 제어권 양도

    // GPIO 146: 이더넷 송수신 활동 상태 표시용 LED (CM 코어)
    GPIO_setPinConfig(GPIO_146_GPIO146);
    GPIO_setPadConfig(146u, GPIO_PIN_TYPE_STD);
    GPIO_writePin(146u, 0u); // 초기 상태 OFF (Active High 기준)
    GPIO_setDirectionMode(146u, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(146u, GPIO_CORE_CM); // CM으로 제어권 양도
}

/*
@funtion    static void InitialPeripherals(void)
@brief      DSP 주변 디바이스 초기화
@param      void
@return     static void
@remark 
    - ADC, PWM, UI, 통신 채널 등을 일괄 초기화하는 래퍼 함수입니다.
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
@brief      ADC 등 아날로그 입력 하드웨어 초기화
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
@brief      EPWM 등 펄스 폭 변조 제어 하드웨어 초기화
@param      void
@return     static void
*/
static void initSystemPwm(void)
{
    initEPWM8();
    initEPWM9(); // 온도 센서 전용 1kHz 느린 주기 ADC 트리거용 ePWM9 추가 기동
    Initial_Epwm7a();
}

/*
@funtion    static void initSystemUserInterface(void)
@brief      사용자 인터페이스 장치(LED 등) 초기화
@param      void
@return     static void
*/
static void initSystemUserInterface(void)
{
    Initial_LED();
}

/*
@funtion    static void initSystemCommunications(void)
@brief      SPI, SCI, Timer 등 시스템 통신 초기화
@param      void
@return     static void
*/
static void initSystemCommunications(void)
{
    Initial_SPI();
    Initial_SCI();
    Initial_TIMER();
    // Initial_IPC(); // IPC 동기화 타이밍 매칭을 위해 main.c의 DSP_Initialization() 직후로 호출 이동
    Initial_EpwmTimer();  /* EPWM1 기반 2ms UDP TX 타이머 활성화 */
}

/*
@funtion    static void initEmacGpioPins(void)
@brief      EMAC MII 모드 GPIO 핀 MUX 설정 (CPU1 전용 제어 필요)
@param      void
@return     static void
@remark
    - 회로도(U17, DP83822) 기준 GPIO 핀 매핑:
        TX_CLK:  GPIO44,  TX_EN:  GPIO118
        TX_D0:   GPIO75,  TX_D1:  GPIO122, TX_D2:  GPIO123, TX_D3: GPIO124
        RX_CLK:  GPIO111, RX_DV:  GPIO112, RX_ER:  GPIO113
        RX_D0:   GPIO114, RX_D1:  GPIO115, RX_D2:  GPIO116, RX_D3: GPIO117
        CRS:     GPIO109, COL:    GPIO110
        MDC:     GPIO105, MDIO:   GPIO106
        INT/PWDN:GPIO108 (입력, Active-Low 인터럽트)
        RESET:   GPIO119 (출력, Active-Low 리셋)
    - EMAC 하드웨어 페리페럴 핀은 GPIO_setMasterCore() 불필요
      (EMAC이 직접 핀을 구동하며 GPIO 데이터 레지스터를 사용하지 않음)
    - GPIO MUX 설정 후 실제 EMAC 드라이버 초기화는 CM 코어 hal_Ethernet.c 에서 수행합니다.
*/
static void initEmacGpioPins(void)
{
    /* --- 이더넷 모듈 클럭 공급 설정 (공식 예제 기준 100MHz 세팅) --- */
    SysCtl_setEnetClk(SYSCTL_ENETCLKOUT_DIV_2, SYSCTL_SOURCE_SYSPLL);

    /* --- TX 경로 --- */
    GPIO_setPinConfig(GPIO_44_ENET_MII_TX_CLK);    /* TX 클럭 */
    GPIO_setPinConfig(GPIO_118_ENET_MII_TX_EN);    /* TX Enable */
    GPIO_setPinConfig(GPIO_75_ENET_MII_TX_DATA0);  /* TX Data bit0 */
    GPIO_setPinConfig(GPIO_122_ENET_MII_TX_DATA1); /* TX Data bit1 */
    GPIO_setPinConfig(GPIO_123_ENET_MII_TX_DATA2); /* TX Data bit2 */
    GPIO_setPinConfig(GPIO_124_ENET_MII_TX_DATA3); /* TX Data bit3 */

    /* --- RX 경로 --- */
    GPIO_setPinConfig(GPIO_111_ENET_MII_RX_CLK);   /* RX 클럭 */
    GPIO_setPinConfig(GPIO_112_ENET_MII_RX_DV);    /* RX Data Valid */
    GPIO_setPinConfig(GPIO_113_ENET_MII_RX_ERR);   /* RX Error */
    GPIO_setPinConfig(GPIO_114_ENET_MII_RX_DATA0); /* RX Data bit0 */
    GPIO_setPinConfig(GPIO_115_ENET_MII_RX_DATA1); /* RX Data bit1 */
    GPIO_setPinConfig(GPIO_116_ENET_MII_RX_DATA2); /* RX Data bit2 */
    GPIO_setPinConfig(GPIO_117_ENET_MII_RX_DATA3); /* RX Data bit3 */

    /* --- MDIO 관리 인터페이스 --- */
    GPIO_setPinConfig(GPIO_105_ENET_MDIO_CLK);     /* MDC 클럭 */
    GPIO_setPinConfig(GPIO_106_ENET_MDIO_DATA);    /* MDIO 데이터 */

    /* --- CRS / COL 선택적 MII 신호 --- */
    GPIO_setPinConfig(GPIO_109_ENET_MII_CRS);      /* CRS/CRS_DV */
    GPIO_setPinConfig(GPIO_110_ENET_MII_COL);      /* COL/GPIO2 */

    /* --- PWDN/INT 핀 (GPIO108): 원래대로 일반 GPIO 입력 및 풀업으로 원복 --- */
    GPIO_setPinConfig(GPIO_108_GPIO108);
    GPIO_setDirectionMode(108U, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(108U, GPIO_PIN_TYPE_PULLUP);

    /* --- PHY 하드웨어 리셋 (GPIO119, Active-Low) --- */
    GPIO_setPinConfig(GPIO_119_GPIO119);
    GPIO_setDirectionMode(119U, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(119U, GPIO_PIN_TYPE_STD);      /* 강력한 3.3V 방출을 위해 푸시풀 모드 유지 */
    GPIO_writePin(119U, 0U);                         /* Active-Low 리셋 강제 인가 (0V) */
    DEVICE_DELAY_US(10000U);                         /* 10ms 대기 (PHY 리셋 타이밍 최소 폭 충족) */
    GPIO_writePin(119U, 1U);                         /* 리셋 완벽 해제 (3.3V) ➡️ PHY 물리 칩 기동! */
}

/*
@funtion    static void Initial_CmCore(void)
@brief      CM 코어 부팅 및 클럭 설정
@param      void
@return     static void
*/
static void Initial_CmCore(void)
{
    // CM 클럭 활성화 (AUXPLL 기반 125MHz 설정)
    SysCtl_setCMClk(SYSCTL_CMCLKOUT_DIV_1, SYSCTL_SOURCE_AUXPLL);

#ifdef _FLASH
    // Flash 다운로드 디버깅 환경에 맞추어 CM 부트 모드를 Flash Sector0로 지정
    Device_bootCM(BOOTMODE_BOOT_TO_FLASH_SECTOR0);
#else
    // RAM 디버깅 환경에 맞추어 CM 부트 모드를 S0RAM으로 지정
    Device_bootCM(BOOTMODE_BOOT_TO_S0RAM);
#endif
}
