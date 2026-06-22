/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Ipc_cpu1.c
    Version          : 00.03
    Description      : CM Core IPC Device Driver 및 공유 메모리 설정
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비 및 인클루드 수정)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (중복 인클루드 제거 및 이력 보완)
 * 2026. 06. 22. - GSRAM 잔재 주석을 MSGRAM 기준으로 수정
 * 2026. 06. 22. - CPU1TOCM MSGRAM 사용으로 인해 불필요해진 Initial_IPC_Mastership 설정 변경 이력 기재
 * 2026. 06. 22. - 이더넷 데이터 폴링 방식으로 변경되어 isrIpcFromCM에서 recvIpcCmMessage 호출 분기 제거
 * 2026. 06. 22. - MSGRAM 롤백으로 인해 불필요해진 Initial_IPC_Mastership 함수 완전 삭제
 */

#include "hal_Ipc_cpu1.h"

/* 구조체 할당 */
volatile stIpcState xIpcState = {false}; // CM 코어 기동 완료 여부 플래그

/* 정적 ISR 선언 */
static __interrupt void isrIpcFromCM(void);


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
            xIpcState.isCmReady = true;
        }
        else
        {
            /* 방어적 default 분기: 정적분석 DAPA SCR-G 만족용 */
        }

        IPC_ackFlagRtoL(IPC_CPU1_L_CM_R, IPC_FLAG0);
    }

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP11);
}
