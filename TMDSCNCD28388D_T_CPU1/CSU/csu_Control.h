/**********************************************************************
 Nexcom Co., Ltd.
 Filename         : csu_Control.h
 Version          : 00.00
 Description      : 시스템 메인 제어 및 인터럽트 로직 선언
 Programmer       : Kim Jeonghwan
 Last Updated     : 2026. 06. 19. (신규 생성)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - 신규 생성
 */

#ifndef CSU_CONTROL_H_
#define CSU_CONTROL_H_

#include "main_cpu1.h"

void Control_Init(void);
__interrupt void MainControl_Isr(void);

#endif /* CSU_CONTROL_H_ */
