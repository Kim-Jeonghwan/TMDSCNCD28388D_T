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
#include "DevIPC.h"
#include "DevEthernet.h"
#include "DevTimer.h"

/* CSU 계층 */
#include "CSU_IPC.h"
#include "CSU_Ethernet.h"

#endif // MAIN_H
