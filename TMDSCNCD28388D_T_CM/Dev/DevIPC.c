/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevIPC.c
    Description      : CM Core IPC Device Driver
    Last Updated     : 2026. 05. 29. (정적 시험 기준 준수 및 함수 주석 보강)
**********************************************************************/

#include "DevIPC.h"

static __interrupt void isrIpcFromCPU1(void);

/*
@funtion    void Initial_IPC(void)
@brief      CM 코어 IPC 인터럽트(IPC1) 등록 및 CPU1 코어 동기화
@param      void
@return     void
@remark
    - CPU1으로부터의 인터럽트를 활성화하고 FLAG31을 사용해 코어 간 구동 타이밍을 맞춥니다.
*/
void Initial_IPC(void)
{
    // 1. CPU1으로부터 수신받을 인터럽트 등록
    IPC_registerInterrupt(IPC_CM_L_CPU1_R, IPC_INT1, isrIpcFromCPU1);

    // 2. CPU1 코어와 동기화 수행
    // CPU1이 준비될 때까지 대기하며 상호 확인합니다.
    IPC_sync(IPC_CM_L_CPU1_R, IPC_FLAG31);
}

/*
@funtion    static __interrupt void isrIpcFromCPU1(void)
@brief      CPU1 코어로부터 수신된 IPC 인터럽트 서비스 루틴 (IPC1)
@param      void
@return     static __interrupt void
@remark
    - CPU1이 보낸 명령과 데이터를 읽어 CSU_IPC 레이어로 처리 요청을 넘기고 수신 플래그를 ack합니다.
*/
static __interrupt void isrIpcFromCPU1(void)
{
    uint32_t command, addr, data;
    bool status;

    // CPU1이 보낸 FLAG1 명령 읽기
    status = IPC_readCommand(IPC_CM_L_CPU1_R, IPC_FLAG1, IPC_ADDR_CORRECTION_DISABLE, &command, &addr, &data);

    if(status == true)
    {
        // CSU_IPC 핸들러 호출
        recvIpcCpu1Message(command, addr, data);

        // 플래그 승인 처리 (CPU1의 FLAG1 Busy 해제)
        IPC_ackFlagRtoL(IPC_CM_L_CPU1_R, IPC_FLAG1);
    }
}

/*
@funtion    void sendIpcMessageToCPU1(uint32_t command, uint32_t addr, uint32_t data)
@brief      CPU1 코어로 IPC 명령 및 데이터 패킷 송신
@param      uint32_t command: 전송할 명령어 코드
@param      uint32_t addr: 전송할 주소 정보
@param      uint32_t data: 전송할 일반 데이터 값
@return     void
@remark
    - CPU1의 수신 대기 플래그(FLAG0)가 해제될 때까지 대기한 후 패킷 전송을 진행합니다.
*/
void sendIpcMessageToCPU1(uint32_t command, uint32_t addr, uint32_t data)
{
    // CPU1은 FLAG0을 모니터링하여 수신함
    while(IPC_isFlagBusyLtoR(IPC_CM_L_CPU1_R, IPC_FLAG0) == true)
    {
        // 대기
    }

    // FLAG0을 사용하여 명령 송신
    IPC_sendCommand(IPC_CM_L_CPU1_R, IPC_FLAG0, IPC_ADDR_CORRECTION_DISABLE, command, addr, data);
}
