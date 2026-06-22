/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : csu_Control.h
    Version          : 00.00
    Description      : 시스템 메인 제어 및 인터럽트 로직 선언
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비 및 매크로 이동)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (매크로 상수 헤더 이동 및 이력 보완)
 * 2026. 06. 19. - 신규 생성
 */

#ifndef CSU_CONTROL_H_
#define CSU_CONTROL_H_

#include "main_cpu1.h"

/* ************************** [[   define   ]]  *********************************************************** */
#define SINE_WAVE_STEP 0.000314159f // 0.5Hz = 100us * 20000 step (PC 모니터링 시각화용)

void Control_Init(void);
__interrupt void MainControl_Isr(void);

#endif /* CSU_CONTROL_H_ */
