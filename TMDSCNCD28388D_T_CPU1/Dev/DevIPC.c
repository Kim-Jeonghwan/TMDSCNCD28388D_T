/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevIPC.c
    Description      : CM Core IPC Device Driver 및 공유 메모리 설정
    Last Updated     : 2026. 05. 29. (정적 시험 기준 준수 및 함수 주석 보강)
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

    // CM 코어와 동기화 수행
    IPC_sync(IPC_CPU1_L_CM_R, IPC_FLAG31);
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
    uint32_t command, addr, data;
    bool status;

    // 명령 읽기
    status = IPC_readCommand(IPC_CPU1_L_CM_R, IPC_FLAG0, IPC_ADDR_CORRECTION_DISABLE, &command, &addr, &data);

    if(status == true)
    {
        // CSU_IPC에서 메시지 처리 실행
        recvIpcCmMessage(command, addr, data);

        // 플래그 승인(Acknowledge) 처리
        IPC_ackFlagRtoL(IPC_CPU1_L_CM_R, IPC_FLAG0);
    }

    // IPC용 PIE ACK 클리어 (CM-to-CPU IPC0은 Group 11에 속함)
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP11);
}

/*
@funtion    void sendIpcMessageToCM(uint32_t command, uint32_t addr, uint32_t data)
@brief      CM 코어로 IPC 명령 및 데이터 패킷 전송
@param      uint32_t command: 전송할 명령어 코드
@param      uint32_t addr: 전송 주소 또는 보조 데이터
@param      uint32_t data: 전송할 일반 데이터
@return     void
@remark
    - 이전 전송이 완료되어 플래그가 클리어될 때까지 대기한 후 신규 명령 패킷을 송출합니다.
*/
void sendIpcMessageToCM(uint32_t command, uint32_t addr, uint32_t data)
{
    // IPC 송신 플래그가 해제될 때까지 대기
    while(IPC_isFlagBusyLtoR(IPC_CPU1_L_CM_R, IPC_FLAG1) == true)
    {
        // 대기 (아무 작업도 하지 않음)
    }

    // 명령 전송 실행
    IPC_sendCommand(IPC_CPU1_L_CM_R, IPC_FLAG1, IPC_ADDR_CORRECTION_DISABLE, command, addr, data);
}
