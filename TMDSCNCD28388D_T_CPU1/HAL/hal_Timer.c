/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Timer.c
    Version          : 00.10
    Description      : CPU1 시스템 주기 타이머 (CPUTimer 0, 1, 2) 드라이버 소스
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (작성자 기입 및 이력 블록 보완)
 * 2026. 06. 04. - 100us CPUTimer0 인터럽트 내 SCI 블로킹 송신(sendScia_SCI_PC) 제거
 */


/* ************************** [[   include  ]]  *********************************************************** */
#include "hal_Timer.h"


/* ************************** [[   define   ]]  *********************************************************** */


/* ************************** [[   global   ]]  *********************************************************** */
stTimer xTimer;


/* ************************** [[  static prototype  ]]  *************************************************** */
static void initCPUTimers(void);

static void configCPUTimer(uint32_t cpuTimer, float32_t freq, float32_t period);


/* ************************** [[  function  ]]  *********************************************************** */
/*
@funtion    void Initial_TIMER(void)
@brief      CPU1 코어의 하드웨어 타이머 초기화 (0, 1, 2)
@param      void
@return     void
@remark 
    - 인터럽트 매핑 후 타이머 주기(100us, 1ms, 1s)를 설정하고 카운트를 시작합니다.
*/
void Initial_TIMER(void)
{
	//	  
	// 각 CPU 타이머 인터럽트의 ISR 등록
	//    
	Interrupt_register(INT_TIMER0, &isr_CpuTimer0);
    Interrupt_register(INT_TIMER1, &isr_CpuTimer1);
    Interrupt_register(INT_TIMER2, &isr_CpuTimer2);

	//
	// 디바이스 주변 장치를 초기화합니다. 여기서는 CPU 타이머만 초기화합니다.
	//
	initCPUTimers();

	//
	// CPU 타이머 0, 1, 2가 각각 100us, 1ms, 1000ms 주기로 인터럽트를 발생하도록 구성합니다.
	//
	configCPUTimer(CPUTIMER0_BASE, DEVICE_SYSCLK_FREQ, 100.0f);	 	// 100 us
	configCPUTimer(CPUTIMER1_BASE, DEVICE_SYSCLK_FREQ, 1000.0f);		// 1 ms
	configCPUTimer(CPUTIMER2_BASE, DEVICE_SYSCLK_FREQ, 1000000.0f);	// 1000 ms


	//
	// CPU 타이머 0, 1, 2에 연결된 CPU 인터럽트들을 활성화합니다.
	// PIE에서 TINT0(Group 1, Interrupt 7)을 활성화합니다.
	//
	Interrupt_enable(INT_TIMER0);
	Interrupt_enable(INT_TIMER1);
	Interrupt_enable(INT_TIMER2);

	//
	// CPU 타이머 0, 1, 2를 구동(카운트 시작)합니다.
	//
	CPUTimer_startTimer(CPUTIMER0_BASE);
	CPUTimer_startTimer(CPUTIMER1_BASE);
	CPUTimer_startTimer(CPUTIMER2_BASE);


}

//
// initCPUTimers - 3개의 CPU 타이머를 모두 알려진 기본 상태로 초기화합니다.
//
/*
@funtion    static void initCPUTimers(void)
@brief      3개의 하드웨어 CPU 타이머 모듈의 기본 레지스터 설정 초기화
@param      void
@return     static void
@remark
    - 타이머 주기를 최대(0xFFFFFFFF)로 설정하고, 1분주(SYSCLKOUT) 분주기 및 타이머 정지 처리를 수행합니다.
*/
static void initCPUTimers(void)
{
    //
    // 타이머 주기를 최댓값으로 초기화
    //
    CPUTimer_setPeriod(CPUTIMER0_BASE, 0xFFFFFFFF);
    CPUTimer_setPeriod(CPUTIMER1_BASE, 0xFFFFFFFF);
    CPUTimer_setPeriod(CPUTIMER2_BASE, 0xFFFFFFFF);

    //
    // 프리스케일 카운터를 1분주(SYSCLKOUT)로 초기화
    //
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0u);
    CPUTimer_setPreScaler(CPUTIMER1_BASE, 0u);
    CPUTimer_setPreScaler(CPUTIMER2_BASE, 0u);

    //
    // 타이머가 정지된 상태인지 확인
    //
    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_stopTimer(CPUTIMER1_BASE);
    CPUTimer_stopTimer(CPUTIMER2_BASE);

    //
    // 모든 카운터 레지스터를 주기값으로 로드
    //
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER1_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER2_BASE);

    //
    // 인터럽트 카운터 및 구조체 리셋
    //
	(void)memset(&xTimer, 	0u, sizeof(xTimer));		//  타이머 변수 초기화
}



//
// configCPUTimer - 지정된 타이머를 주파수(Hz) 및 주기(us)에 맞춰 초기화하고, 설정 완료 후 정지 상태로 유지합니다.
//
/*
@funtion    static void configCPUTimer(uint32_t cpuTimer, float32_t freq, float32_t period)
@brief      개별 CPU 타이머의 동작 주파수 및 인터럽트 발생 주기 설정
@param      uint32_t cpuTimer: 설정 대상 타이머 레지스터 베이스 주소
@param      float32_t freq: 입력 시스템 클럭 주파수 (Hz)
@param      float32_t period: 원하는 인터럽트 발생 주기 (us)
@return     static void
@remark
    - 입력 주기(us)에 도달할 때 인터럽트를 발생시키도록 Period 레지스터 값을 산출하여 로드합니다.
*/
static void configCPUTimer(uint32_t cpuTimer, float32_t freq, float32_t period)
{
    uint32_t temp;

    //
    // 타이머 주기 초기화:
    //
    temp = (uint32_t)((freq / 1000000.0f) * period);
    CPUTimer_setPeriod(cpuTimer, temp - 1u);

    //
    // 프리스케일 카운터를 1분주(SYSCLKOUT)로 설정:
    //
    CPUTimer_setPreScaler(cpuTimer, 0u);

    //
    // 타이머 제어 레지스터를 초기화합니다. 타이머를 정지 및 재로드하고,
    // Free Run 비활성화 및 인터럽트를 활성화합니다. 추가적으로 Emulation 비트를 설정합니다.
    //
    CPUTimer_stopTimer(cpuTimer);
    CPUTimer_reloadTimerCounter(cpuTimer);
    CPUTimer_setEmulationMode(cpuTimer,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    CPUTimer_enableInterrupt(cpuTimer);
}





/*
@funtion    __interrupt void isr_CpuTimer0(void)
@brief      CPU 타이머 0 인터럽트 서비스 루틴 (100us)
@param      void
@return     __interrupt void
@remark 
    - 초고속 처리가 필요한 100us 주기의 작업을 예약하기 위한 필드입니다.
*/
__interrupt void isr_CpuTimer0(void)
{
    //
    // Group 1 인터럽트를 다시 받기 위해 ACK를 승인합니다.
    //

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}


/*
@funtion    __interrupt void isr_CpuTimer1(void)
@brief      CPU 타이머 1 인터럽트 서비스 루틴 (1ms)
@param      void
@return     __interrupt void
@remark 
    - 백그라운드 태스크 스케줄링을 위한 1ms, 10ms, 100ms, 1000ms 계수 카운트를 증가시킵니다.
*/
__interrupt void isr_CpuTimer1(void)
{
	xTimer.Cycle_1ms ++;
	xTimer.Cycle_10ms ++;
	xTimer.Cycle_100ms ++;
	xTimer.Cycle_1000ms ++;

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}



/*
@funtion    __interrupt void isr_CpuTimer2(void)
@brief      CPU 타이머 2 인터럽트 서비스 루틴 (1000ms = 1s)
@param      void
@return     __interrupt void
@remark 
    - 시스템 디버깅용 동작 주파수(Hz)를 측정합니다.
*/
__interrupt void isr_CpuTimer2(void)
{
    //
    // CPU 인터럽트 승인 처리
    //

    xTimer.Hz = xTimer.Hzcnt;
    xTimer.Hzcnt = 0u;

	Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
