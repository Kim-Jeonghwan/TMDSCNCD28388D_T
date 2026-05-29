/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_eQEP.c
    Description      : 
    Last Updated     : 2026. 05. 29. (정적 시험 기준 준수 및 함수 주석 보강)
**********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "CSU_eQEP.h"



/* ************************** [[   define   ]]  *********************************************************** */
#define GPIO_ZEROSETQEP_SWITCH         22u


/* ************************** [[   global   ]]  *********************************************************** */



/* ************************** [[  static prototype  ]]  *************************************************** */




/* ************************** [[  function  ]]  *********************************************************** */




/*
@funtion    void Init_Eqep1Gpio(void)
@brief      eQEP1용 GPIO 핀 및 노이즈 필터링 설정
@param      void
@return     void
@remark
    - GPIO 20(A상), GPIO 21(B상)을 eQEP 기능으로 할당하고 6샘플 필터를 적용합니다.
    - Zero 위치 설정을 위한 Tact 스위치용 GPIO 22번 입력을 설정합니다.
*/
void Init_Eqep1Gpio(void)
{
    EALLOW;

    // GPIO 20 : EQEP1A (A상) 설정
    GPIO_setPinConfig(GPIO_20_EQEP1_A);               // 20번 핀을 EQEP1A 기능으로 할당
    GPIO_setPadConfig(20, GPIO_PIN_TYPE_STD);          // 외부 인버터가 있으므로 내부 풀업 없이 표준 설정
    GPIO_setQualificationMode(20, GPIO_QUAL_6SAMPLE);     // 노이즈 제거를 위해 6샘플 필터링 적용

    // GPIO 21 : EQEP1B (B상) 설정
    GPIO_setPinConfig(GPIO_21_EQEP1_B);               // 21번 핀을 EQEP1B 기능으로 할당
    GPIO_setPadConfig(21, GPIO_PIN_TYPE_STD);          
    GPIO_setQualificationMode(21, GPIO_QUAL_6SAMPLE);     

    // HW SWITCH (GPIO65) - ZEROSETQEP SWITCH
    GPIO_SetupPinMux(GPIO_ZEROSETQEP_SWITCH, GPIO_MUX_CPU1, 0u);
    GPIO_SetupPinOptions(GPIO_ZEROSETQEP_SWITCH, GPIO_INPUT, GPIO_PULLUP);

    EDIS;
}

/*
@funtion    void Init_Eqep1(void)
@brief      eQEP1 하드웨어 모듈 설정 초기화
@param      void
@return     void
@remark
    - Quadrature 2배 해상도 및 최대 해상도 한도를 기준으로 모듈을 활성화합니다.
*/
void Init_Eqep1(void)
{
// 1. eQEP1 모듈 클럭 활성화 (함수 내부에 포함)
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EQEP1); 

    // 2. 모듈 비활성화 후 설정 시작
    EQEP_disableModule(EQEP1_BASE);
    
    // 3. Quadrature 모드 및 해상도 설정
    EQEP_setDecoderConfig(EQEP1_BASE, (EQEP_CONFIG_QUADRATURE | 
                                       EQEP_CONFIG_2X_RESOLUTION | 
                                       EQEP_CONFIG_NO_SWAP));
    
    // 4. 포지션 카운터 최대치 설정 (누적 카운트용)
    EQEP_setPositionCounterConfig(EQEP1_BASE, EQEP_POSITION_RESET_MAX_POS, QEP_RESOLUTION_PER_REV - 1);
    // 5. 모듈 활성화
    EQEP_enableModule(EQEP1_BASE);
}


/*
@funtion    void EqeptoEncoder(void)
@brief      eQEP1 카운터 값을 각도 및 Raw 펄스 데이터로 변환
@param      void
@return     void
@remark
    - 실시간 엔코더 위치를 계산하여 SCI PC 전송 구조체에 적재합니다.
*/
void EqeptoEncoder(void)
{
	// 1. 각도 계산
    xXmtSciPcMsg1.EncoderAngle = (uint16_t)(EQEP_getPosition(EQEP1_BASE) * (360.0f / 96.0f) * 100.0f + 0.5f); // +0.5f : 소수 1자리 반올림 위함
        
    // 2. RawPD 계산
    xXmtSciPcMsg1.EncoderRawPD = (uint16_t)EQEP_getPosition(EQEP1_BASE);
}


/*
@funtion    void updateHwSwitchStatus2(void)
@brief      하드웨어 영점 리셋 스위치 상태 주기 업데이트
@param      void
@return     void
@remark
    - GPIO 22 스위치 입력 시 eQEP 카운터 및 송신 값을 영점으로 즉시 초기화합니다.
*/
void updateHwSwitchStatus2(void)
{
    // GPIO 22번이 1(High)이면 eQEP 카운터 자체를 0으로 리셋
    if(GPIO_readPin(GPIO_ZEROSETQEP_SWITCH) == true)
    {
        EQEP_setPosition(EQEP1_BASE, 0u); // 하드웨어 카운터 레지스터를 0으로 설정
        
        // 구조체 값도 즉시 0으로 초기화
        xXmtSciPcMsg1.EncoderAngle = 0u;
        xXmtSciPcMsg1.EncoderRawPD = 0u;
    }
}
