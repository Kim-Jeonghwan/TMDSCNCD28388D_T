/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_Adc.c
    Description      : ADC Application Logic (10ms Periodic Task)
    Last Updated     : 2026. 06. 01. (DSP 내부 온도 센서 수집 소스 및 소수점 정밀 연산 공식 적용)
***********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "CSU_Adc.h"

/* ************************** [[   global   ]]  *********************************************************** */
// DevAdc.c에 선언된 실시간 온도 센서 원시 결과 전역 변수 공유 참조
extern uint16_t adcResult;

// 디버깅 및 실시간 표출용 온도 센서 결과 전역 변수 (타입 미스매치 방지를 위해 float32_t로 선언)
float32_t currentTemperatureC = 0.0f;
uint32_t debugAdcLoopCnt = 0u; // 소프트웨어 실행 검증용 카운터

// C2000Ware OTP 캘리브레이션 셋업 전역 변수 참조
extern float32 tempSensor_tempSlope;
extern float32 tempSensor_tempOffset;
extern float32 tempSensor_scaleFactor;

extern void InitTempSensor(float32_t vrefhi_voltage);


/* ************************** [[  static prototype  ]]  *************************************************** */
static void updateDspTempSensor(void);


/* ************************** [[  function  ]]  *********************************************************** */

/**
 * @brief ADC 애플리케이션 초기화
 */
void Initial_Adc(void)
{
    // DSP 내부 온도 센서 활성화(캘리브레이션 초기화, 외부 레퍼런스 3.3V 적용)
    InitTempSensor(3.3f);
}


/**
 * @brief ADC 데이터 업데이트 (10ms 주기 호출)
 * @details 전체 ADC 모듈의 주기적 갱신을 수행하며, 하위 센서 모듈들을 차례로 업데이트합니다.
 */
void updateAdcData(void)
{
    // DSP 내부 온도 센서 데이터 업데이트
    updateDspTempSensor();
}


/**
 * @brief DSP 내부 온도 센서 데이터 측정 및 섭씨 온도 변환
 * @details ePWM8 자동 기동 인터럽트가 백그라운드에서 실시간 취득한 원시 결과(adcResult)를 섭씨로 변환합니다.
 *          소수점 이하 정밀도 유지를 위해 실수 연산을 직접 적용하며, Divide by Zero 방어를 수행합니다.
 */
static void updateDspTempSensor(void)
{
    // 소프트웨어 실행 검증용 카운터 증가 (주기 구동 검증용)
    debugAdcLoopCnt++;

    // C2000Ware 내장 원본 소수점 계산 공식 직접 적용 (12비트 기준 최대 4096 분해능 분모 적용)
    // Divide by Zero 취약점 (CWE-369) 방지를 위한 사전 분모 조건 검사 수행
    if (tempSensor_tempSlope != 0.0f)
    {
        currentTemperatureC = ((((float32_t)adcResult * tempSensor_scaleFactor / 4096.0f) - tempSensor_tempOffset) / tempSensor_tempSlope);
    }
    else
    {
        currentTemperatureC = 0.0f; // 슬로프 값이 비정상일 경우 안전 디폴트 값 할당
    }
}
