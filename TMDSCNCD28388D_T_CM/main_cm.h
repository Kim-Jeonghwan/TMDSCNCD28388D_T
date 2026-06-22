/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main_cm.h
    Version          : 00.02
    Description      : CM 전역 헤더 관리 파일
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 22. (물리 파일명 소문자화에 맞춰 인클루드 정비 확인)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 22. - 물리 파일명 소문자화에 맞춰 인클루드 상태 확인 및 버전 갱신
 * 2026. 06. 19. - 코드 스타일 및 주석 템플릿 적용
 * 2026. 04. 22. - 초기 작성
 */

#ifndef MAIN_CM_H
#define MAIN_CM_H

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
#include "hal_Ipc_cm.h"
#include "hal_Ethernet.h"
#include "hal_Timer.h"

/* CSU 계층 */
#include "csu_Ipc_cm.h"
#include "csu_Ethernet.h"

#endif // MAIN_CM_H
