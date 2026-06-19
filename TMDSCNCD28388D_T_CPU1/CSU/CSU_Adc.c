/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Adc.c
    Version          : 00.01
    Description      : CPU1 ADC Application Logic
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (코드 스타일 및 구조체 템플릿 적용)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - 코드 스타일 및 구조체 템플릿 적용
 * 2026. 06. 05. - 코드 주석 포맷팅 및 한글화
 */

/* ************************** [[   include  ]]  *********************************************************** */
#include "csu_Adc.h"

/* ************************** [[   global   ]]  *********************************************************** */
// hal_Adc.c에 선언된 실시간 온도 센서 원시 결과 전역 변수 공유 참조
extern uint16_t adcResult;

// 구조체 변수 선언
stAdcState xAdc = {0};

// C2000Ware OTP 캘리브레이션 셋업 전역 변수 참조
extern float32 tempSensor_tempSlope;
extern float32 tempSensor_tempOffset;
extern float32 tempSensor_scaleFactor;

extern void InitTempSensor(float32_t vrefhi_voltage);


/* ************************** [[  static prototype  ]]  *************************************************** */
static void updateDspTempSensor(void);


/* ************************** [[  function  ]]  *********************************************************** */

/*
@function    Initial_Adc
@brief      ADC 애플리케이션 초기화
@param      void
@return     void
@remark
    - DSP 내부 온도 센서 캘리브레이션을 수행합니다.
*/
void Initial_Adc(void)
{
    // DSP 내부 온도 센서 활성화(캘리브레이션 초기화, 외부 레퍼런스 3.3V 적용)
    InitTempSensor(3.3f);
}


/*
@function    updateAdcData
@brief      ADC 데이터 업데이트 (10ms 주기 호출)
@param      void
@return     void
@remark
    - 하위 센서 모듈들을 차례로 갱신하며, 현재 내부 온도 센서 데이터를 수집합니다.
*/
void updateAdcData(void)
{
    // DSP 내부 온도 센서 데이터 업데이트
    updateDspTempSensor();
}


/*
@function    updateDspTempSensor
@brief      DSP 내부 온도 센서 데이터 측정 및 섭씨 온도 변환
@param      void
@return     static void
@remark
    - ePWM9 이벤트로 수집된 ADC 원시 결과를 섭씨(C)로 변환합니다.
    - 소수점 이하 정밀도 유지를 위해 실수 연산을 직접 적용하며, Divide by Zero 취약점을 방어합니다.
*/
static void updateDspTempSensor(void)
{

    // C2000Ware 내장 원본 소수점 계산 공식 직접 적용 (12비트 기준 최대 4096 분해능 분모 적용)
    // Divide by Zero 취약점 (CWE-369) 방지를 위한 사전 분모 조건 검사 수행
    if (tempSensor_tempSlope != 0.0f)
    {
        float32_t rawTempC = ((((float32_t)adcResult * tempSensor_scaleFactor / 4096.0f) - tempSensor_tempOffset) / tempSensor_tempSlope);
        
        // IIR 로우패스 필터 적용 (노이즈로 인한 1의 자리 및 소수점 자리 요동 방지)
        // Alpha = 0.05 (새로운 값 5%, 기존 값 95% 반영)
        if (xAdc.currentTemperatureC == 0.0f)
        {
            xAdc.currentTemperatureC = rawTempC; // 초기화
        }
        else
        {
            xAdc.currentTemperatureC = (xAdc.currentTemperatureC * 0.95f) + (rawTempC * 0.05f);
        }
    }
    else
    {
        xAdc.currentTemperatureC = 0.0f; // 슬로프 값이 비정상일 경우 안전 디폴트 값 할당
    }
}
