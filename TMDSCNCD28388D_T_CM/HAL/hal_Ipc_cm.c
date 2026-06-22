/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Ipc_cm.c
    Version          : 00.02
    Description      : CM Core IPC Device Driver
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 22. (GSRAM 잔재 주석을 MSGRAM 기준으로 수정)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 22. - GSRAM 잔재 주석을 MSGRAM 기준으로 수정
 * 2026. 06. 22. - CPU1의 송신이 MSGRAM 폴링으로 변경됨에 따라 recvIpcCpu1Message 호출 제거
 */

#include "hal_Ipc_cm.h"

static void isrIpcFromCPU1(void);

/* IPC ISR 호출 진단 카운터 (CCS Expressions 사용에서 확인) */

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
    // 1. CPU1으로부터 수신받을 인터럽트 등록 (NVIC 핸들러 + 인터럽트 활성화)
    IPC_registerInterrupt(IPC_CM_L_CPU1_R, IPC_INT1, isrIpcFromCPU1);

    // 2. [핵심 수정] CPU1의 EPWM ISR이 CM 등록 전에 이미 FLAG1을 SET한 경우 강제 ACK
    //    (NVIC edge-triggered 특성상, 이미 HIGH인 FLAG는 새 ISR을 트리거하지 못함)
    //    ACK으로 FLAG1을 LOW로 내리면, 다음 IPC_sendCommand에서 새 edge 발생 → ISR 정상 트리거
    if (IPC_isFlagBusyRtoL(IPC_CM_L_CPU1_R, IPC_FLAG1))
    {
        IPC_ackFlagRtoL(IPC_CM_L_CPU1_R, IPC_FLAG1);
    }

    // 3. CPU1 코어와 동기화 수행 (CPU1이 준비될 때까지 대기)
    IPC_sync(IPC_CM_L_CPU1_R, IPC_FLAG31);
}

/*
@funtion    static void isrIpcFromCPU1(void)
@brief      CPU1 코어로부터 수신된 IPC 인터럽트 서비스 루틴 (IPC1)
@param      void
@return     static void
@remark
    - CPU1이 보낸 명령과 데이터를 읽어 csu_Ipc_cm 레이어로 처리 요청을 넘기고 수신 플래그를 ack합니다.
*/
static void isrIpcFromCPU1(void)
{
    uint32_t command, addr, data;
    bool status;

    status = IPC_readCommand(IPC_CM_L_CPU1_R, IPC_FLAG1, IPC_ADDR_CORRECTION_DISABLE, &command, &addr, &data);

    if(status == true)
    {
        // [최적화 포인트]: 데이터 복사가 완료되었으므로, 상위 레이어(CSU) 처리를 수행하기 전에
        // 하드웨어 플래그를 먼저 클리어(ACK)하여 CPU1이 다음 주기를 즉시 준비할 수 있도록 채널을 개방합니다.
        IPC_ackFlagRtoL(IPC_CM_L_CPU1_R, IPC_FLAG1);

        // CPU1 -> CM 데이터 송신이 MSGRAM 폴링으로 변경되어 더 이상 IPC 페이로드 파싱을 수행하지 않습니다.
        // recvIpcCpu1Message(command, addr, data);
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
    while(IPC_isFlagBusyLtoR(IPC_CM_L_CPU1_R, IPC_FLAG0) == true)
    {
        /* CPU1이 이전 명령(FLAG0)을 처리하고 Clear해 줄 때까지 대기 */
    }

    // FLAG0을 사용하여 명령 송신
    IPC_sendCommand(IPC_CM_L_CPU1_R, IPC_FLAG0, IPC_ADDR_CORRECTION_DISABLE, command, addr, data);
}
