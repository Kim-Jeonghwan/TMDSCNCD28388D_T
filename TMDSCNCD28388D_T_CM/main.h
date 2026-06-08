/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : main.h
    Description      : CM Core Main Header
    Last Updated     : 2026. 04. 22.
**********************************************************************/

#ifndef MAIN_H
#define MAIN_H

/* 표준 라이브러리 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

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
