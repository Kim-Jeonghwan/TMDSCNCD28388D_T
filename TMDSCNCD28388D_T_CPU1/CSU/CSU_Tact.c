/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : CSU_Tact.c
    Description      : Tact Switch Input Control (GPIO 64, 65)
    Last Updated     : 2026. 04. 17.
**********************************************************************/

/* ************************** [[   include  ]]  *********************************************************** */
#include "CSU_Tact.h"

/* ************************** [[  function  ]]  *********************************************************** */


/**
 * @brief Tact 스위치 제어를 위한 GPIO 방향 및 Mux 설정
 * @details GPIO 64, 65번을 입력으로 설정하고 내부 풀업을 활성화합니다.
 */
void initGpioDinTact(void)
{
    EALLOW;

    // GPIO 64 (Tact 01) 설정
    GPIO_SetupPinMux(GPIO_TACT_01, GPIO_MUX_CPU1, 0u);
    GPIO_SetupPinOptions(GPIO_TACT_01, GPIO_INPUT, GPIO_PULLUP);

    // GPIO 65 (Tact 02) 설정
    GPIO_SetupPinMux(GPIO_TACT_02, GPIO_MUX_CPU1, 0u);
    GPIO_SetupPinOptions(GPIO_TACT_02, GPIO_INPUT, GPIO_PULLUP);

    EDIS;
}

/**
 * @brief Tact 스위치 관련 초기값 설정
 */
void Initial_Tact(void)
{
    xXmtSciPcMsg1.Tact01 = false;
    xXmtSciPcMsg1.Tact02 = false;
}

/**
 * @brief Tact 스위치 상태를 읽어 SCI_PC 송신 구조체에 저장
 * @details GPIO 64번 -> xXmtSciPcMsg1.Tact01
 *          GPIO 65번 -> xXmtSciPcMsg1.Tact02
 */
void updateTactStatus(void)
{
    // GPIO 상태를 직접 읽어 구조체에 반영
    xXmtSciPcMsg1.Tact01 = (bool)GPIO_readPin(GPIO_TACT_01);
    xXmtSciPcMsg1.Tact02 = (bool)GPIO_readPin(GPIO_TACT_02);
}
