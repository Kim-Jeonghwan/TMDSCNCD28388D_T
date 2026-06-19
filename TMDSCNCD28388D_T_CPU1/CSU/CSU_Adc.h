/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Adc.h
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

#ifndef csu_ADC_H
#define csu_ADC_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main_cpu1.h"

/* ************************** [[   define   ]]  *********************************************************** */


/* ************************** [[   enum or struct   ]]  *************************************************** */
typedef struct {
    float32_t currentTemperatureC;
} stAdcState;

/* ************************** [[   global   ]]  *********************************************************** */
extern stAdcState xAdc;


/* ************************** [[  function  ]]  *********************************************************** */
/*
@function   Initial_Adc
@brief      ADC 애플리케이션 초기화
@param      void
@return     void
@remark     DSP 내부 온도 센서 캘리브레이션을 수행합니다.
*/
void Initial_Adc(void);

/*
@function   updateAdcData
@brief      ADC 데이터 업데이트
@param      void
@return     void
@remark     하위 센서 모듈들을 차례로 갱신하며, 현재 내부 온도 센서 데이터를 수집합니다.
*/
void updateAdcData(void);



#endif /* csu_ADC_H */
