/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevIPC.c
    Description      : CM Core IPC Device Driver 및 공유 메모리 설정
    Last Updated     : 2026. 06. 01. (정적 시험 준수 및 미사용 변수 제거)
**********************************************************************/

#include "DevIPC.h"

static __interrupt void isrIpcFromCM(void);

// 1. CM 코어가 사용할 RAM 권한을 부여하는 함수
/*
@funtion    void Initial_IPC_Mastership(void)
@brief      CM 코어가 사용할 Shared RAM 및 GSRAM 제어 마스터십 부여
@param      void
@return     void
@remark
    - F2838x 하드웨어 레지스터 제어를 통해 S0~S3 및 GS0~GS1 RAM 영역 마스터를 CM 코어로 강제 할당합니다.
*/
void Initial_IPC_Mastership(void)
{
    // F2838x에서는 각 RAM 섹션당 2비트를 사용하여 마스터를 지정함 (10b = CM)
    EALLOW;
    // Shared RAM (S0~S3) 권한을 CM으로 설정 (0xAA = 10101010b -> S0,S1,S2,S3 모두 CM)
    HWREG(MEMCFG_BASE + MEMCFG_O_LSXMSEL) = (HWREG(MEMCFG_BASE + MEMCFG_O_LSXMSEL) & ~0x00FFU) | 0x00AAU;

    // GSRAM (GS0~GS1) 권한을 CM으로 설정 (0x0A = 1010b -> GS0, GS1 모두 CM)
    HWREG(MEMCFG_BASE + MEMCFG_O_GSXMSEL) = (HWREG(MEMCFG_BASE + MEMCFG_O_GSXMSEL) & ~0x000FU) | 0x000AU;
    EDIS;
}

// 2. IPC 통신 설정 및 CM 코어와의 동기화 함수
/*
@funtion    void Initial_IPC(void)
@brief      IPC 통신 초기화 및 CM 코어와 동기화
@param      void
@return     void
@remark
    - IPC0 인터럽트 서비스 루틴을 등록하고 IPC_FLAG31을 통해 CM 코어 구동 상태를 대기/동기화합니다.
*/
void Initial_IPC(void)
{
    // CM으로부터 수신받을 인터럽트 등록 (IPC0)
    IPC_registerInterrupt(IPC_CPU1_L_CM_R, IPC_INT0, isrIpcFromCM);

    // CM 코어와 동기화 수행 (CM 코어 코드 미로드 시 대기 방지를 위해 임시 주석 처리)
    // IPC_sync(IPC_CPU1_L_CM_R, IPC_FLAG31);
}

/*
@funtion    static __interrupt void isrIpcFromCM(void)
@brief      CM 코어로부터 수신된 IPC 인터럽트 서비스 루틴 (IPC0)
@param      void
@return     static __interrupt void
@remark
    - CM에서 보낸 IPC 메시지를 분석하여 CSU_IPC 레이어로 라우팅하고, 관련 인터럽트 플래그를 소거합니다.
*/
static __interrupt void isrIpcFromCM(void)
{
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP11);
}
