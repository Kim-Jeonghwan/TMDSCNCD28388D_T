/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main.h
    Version          : 00.01
    Description      : CM 전역 헤더 관리 파일
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 19. (코드 스타일 및 주석 템플릿 적용)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 19. - 코드 스타일 및 주석 템플릿 적용
 * 2026. 04. 22. - 초기 작성
 */

#ifndef MAIN_H
#define MAIN_H

/* 표준 라이브러리 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* C28x 호환용 자료형 정의 */
typedef float float32_t;

/* CM Core Driverlib */
#include "driverlib_cm.h"
#include "cm.h"

/* Dev 계층 */
#include "hal_IPC.h"
#include "hal_Ethernet.h"
#include "hal_Timer.h"

/* CSU 계층 */
#include "csu_IPC.h"
#include "csu_Ethernet.h"

#endif // MAIN_H
