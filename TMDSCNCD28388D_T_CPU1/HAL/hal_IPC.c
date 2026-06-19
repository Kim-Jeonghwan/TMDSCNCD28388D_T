/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_IPC.c
    Description      : CM Core IPC Device Driver 및 공유 메모리 설정
    Last Updated     : 2026. 06. 05. (코드 주석 포맷팅 및 한글화)
**********************************************************************/

#include "hal_IPC.h"
#include "csu_IPC.h"

/* 전역 변수 */
volatile bool g_bCmReady = false; // CM 코어 기동 완료 여부 플래그

/* 정적 ISR 선언 */
static __interrupt void isrIpcFromCM(void);

/* ---------------------------------------------------------------
 * CM 코어가 사용할 RAM 마스터십 부여
 * --------------------------------------------------------------- */
/*
@funtion    void Initial_IPC_Mastership(void)
@brief      CM 코어가 사용할 Shared RAM 및 GSRAM 제어 마스터십 부여
@param      void
@return     void
@remark
    - F2838x에서 각 RAM 섹션당 2비트로 마스터를 지정합니다 (10b = CM).
*/
void Initial_IPC_Mastership(void)
{
    EALLOW;
    /* 
       GSRAM (GS0~GS7)의 마스터십을 CM(Connectivity Manager) 코어로 위임합니다.
       MEMCFG_O_GSXMSEL 레지스터는 각 GSRAM 블록(GS0~GS15)당 1비트로 매핑됩니다.
       - 0 = C28x CPU1/CPU2 소유
       - 1 = CM 소유
       따라서 CM이 사용하는 S0RAM~S3RAM 및 E0RAM(GS4)을 포함한 GS0~GS7 영역 전체를 
       CM 소유로 설정하기 위해 하위 8비트를 모두 1로 세팅(0x00FFU)합니다.
    */
    HWREG(MEMCFG_BASE + MEMCFG_O_GSXMSEL) =
        (HWREG(MEMCFG_BASE + MEMCFG_O_GSXMSEL) & ~0x00FFU) | 0x00FFU;
    EDIS;
}

/* ---------------------------------------------------------------
 * CM 부팅 전 IPC 사전 청소
 * --------------------------------------------------------------- */
/*
@funtion    void Initial_IPC_Clear(void)
@brief      CM 코어 기동 전에 IPC 제어 레지스터 플래그 사전 정리
@param      void
@return     void
*/
void Initial_IPC_Clear(void)
{
    /* IPC 제어 레지스터의 모든 플래그 강제 클리어 (이전 오염 플래그 제거) */
    IPC_init(IPC_CPU1_L_CM_R);
}

/* ---------------------------------------------------------------
 * IPC 통신 초기화
 * --------------------------------------------------------------- */
/*
@funtion    void Initial_IPC(void)
@brief      IPC 통신 초기화 및 CM 코어와 동기화
@param      void
@return     void
@remark
    - IPC_INT0: CM→CPU1 수신 인터럽트 등록
*/
void Initial_IPC(void)
{
    // IPC_init()은 CM 부팅 전에 Initial_IPC_Clear()에서 수행하므로 여기서는 생략합니다.

    /* CM으로부터 수신받을 인터럽트 등록 (IPC_INT0) */
    IPC_registerInterrupt(IPC_CPU1_L_CM_R, IPC_INT0, isrIpcFromCM);

    /* CM 코어와 IPC_FLAG31을 통해 동기화 수행 (CM의 락업 방지) */
    IPC_sync(IPC_CPU1_L_CM_R, IPC_FLAG31);
}


/* ---------------------------------------------------------------
 * CM→CPU1 IPC 수신 ISR
 * --------------------------------------------------------------- */
/*
@funtion    static __interrupt void isrIpcFromCM(void)
@brief      CM 코어로부터 수신된 IPC 인터럽트 서비스 루틴 (IPC_INT0)
@param      void
@return     static __interrupt void
@remark
    - CM에서 IPC_CMD_CM_ETH_RX_DATA 명령 수신 시 공유 수신 버퍼(xRcvEthMsg) 갱신
*/
static __interrupt void isrIpcFromCM(void)
{
    uint32_t uiCmd  = 0U;
    uint32_t uiAddr = 0U;
    uint32_t uiData = 0U;
    bool     bRet   = false;

    bRet = IPC_readCommand(IPC_CPU1_L_CM_R, IPC_FLAG0, IPC_ADDR_CORRECTION_DISABLE, &uiCmd, &uiAddr, &uiData);

    if (bRet)
    {
        if (uiCmd == IPC_CMD_CM_BOOT_READY)
        {
            /* CM 코어 이더넷 및 통신 준비 완료 플래그 활성화 */
            g_bCmReady = true;
        }
        else if (uiCmd == IPC_CMD_CM_ETH_RX_DATA)
        {
            /* CM으로부터 수신된 PC 명령 데이터를 csu_IPC 레이어로 전달 */
            recvIpcCmMessage(uiCmd, uiAddr, uiData);
        }
        else
        {
            /* 방어적 default 분기: 정적분석 DAPA SCR-G 만족용 */
        }

        IPC_ackFlagRtoL(IPC_CPU1_L_CM_R, IPC_FLAG0);
    }

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP11);
}
