# 모터 제어 및 이더넷 통신 부하 최소화를 위한 GSRAM 공유 메모리 전환 계획

현재 IPC 인터럽트를 통해 이루어지고 있는 CPU1과 CM 간의 데이터 교환(이더넷 송수신 데이터)을 GSRAM(GS0, GS1)을 이용한 직접 메모리 접근(Polling) 방식으로 전환하여 시스템 부하를 최소화합니다.

## User Review Required

> [!IMPORTANT]
> **마스터십 분할 검토**
> 기존에는 `0x00FFU`를 통해 GS0~GS7 전체의 제어권을 CM으로 넘겨주었습니다.
> 이 계획에서는 GS0를 CPU1 전용(CPU1 쓰기, CM 읽기)으로 사용하기 위해, GS0의 제어권을 CPU1에 남겨두고 GS1~GS7만 CM에 위임(`0x00FEU`)하도록 수정합니다. 이 변경 사항이 시스템의 다른 부분에 영향을 주지 않는지 확인이 필요합니다.

> [!TIP]
> **양방향 Lock-Free 동기화 (Seqlock & Try-Lock) 도입 완료**
> 메인루프(100us)와 이더넷(100ms) 간의 속도 차이로 인한 데이터 찢김(Tearing)을 완벽하게 방지하기 위해 양방향으로 **Seqlock** 기법을 도입합니다.
> - **[GS0] CPU1 쓰기 / CM 읽기**: CPU1이 쓸 때 카운터를 홀수 $\rightarrow$ 짝수로 변경하며, CM은 짝수일 때만 읽고 도중에 변하면 다시 읽습니다(Spin-Lock).
> - **[GS1] CM 쓰기 / CPU1 읽기**: CM이 이더넷 수신 데이터를 쓸 때 카운터를 홀수 $\rightarrow$ 짝수로 변경합니다. 단, CPU1(100us ISR)은 읽으려 할 때 카운터가 홀수(CM이 쓰는 중)이면 **기다리지 않고 바로 포기(Try-Lock)** 한 뒤 이전 100us의 데이터를 그대로 사용합니다. 이를 통해 모터 제어 ISR의 지연(Jitter)을 0으로 완벽히 보장합니다.

## Proposed Changes

---

### 공통 헤더 및 구조체 변경 (CSU 계층)

#### [MODIFY] csu_Ipc_cpu1.h / csu_Ipc_cm.h
- **내용**: 
  - GS0(CPU1 -> CM) 및 GS1(CM -> CPU1)의 물리적 시작 주소(CPU1 관점 및 CM 관점)를 매크로로 정의합니다.
  - 구조체 `stIpcDataPacket` 내부에 동기화를 위한 `uint32_t seqCount;` 변수를 추가합니다.
  - 기존 MSGRAM 포인터 대신 GS0, GS1을 가리키는 `pxDataCpu1ToCm`, `pxDataCmToCpu1` 전역 포인터를 선언합니다.

#### [MODIFY] csu_Ipc_cpu1.c / csu_Ipc_cm.c
- **내용**:
  - 선언된 포인터 변수에 실제 GS0 및 GS1의 주소를 할당합니다.
  - CM 코어의 경우 `csu_Ipc_cm.c`에 있던 불필요한 IPC 수신 인터럽트 파싱 로직(`pxIpcCpu1ToCm->Payload.TxData` 파싱 후 `xEthApp` 갱신)을 제거합니다.

---

### 하드웨어 추상화 계층 (HAL)

#### [MODIFY] hal_Ipc_cpu1.c
- **내용**: `Initial_IPC_Mastership()` 함수 수정
  - 기존: `HWREG(MEMCFG_BASE + MEMCFG_O_GSXMSEL) ... | 0x00FFU;` (GS0~GS7 CM 할당)
  - 변경: `| 0x00FEU;` (GS0는 0으로 두어 CPU1이 마스터 유지, GS1~GS7은 CM에 할당)
  - 목적: GS0는 CPU1이 쓰고, GS1은 CM이 쓸 수 있도록 하드웨어 권한 분리.

---

### 비즈니스 로직 계층 (CSU) - CPU1

#### [MODIFY] csu_Control.c
- **내용**: `MainControl_Isr()` 함수 수정
  - **GS1 읽기 (Try-Lock 적용)**: 
    ```c
    uint32_t seq0 = pxDataCmToCpu1->seqCount;
    // CM이 쓰는 중(홀수)이 아닐 때만 읽기 시도 (기다리지 않고 Pass하여 실시간성 보장)
    if ((seq0 & 1U) == 0U) {
        uint8_t tempSeq = pxDataCmToCpu1->Payload.RxData.seqNum;
        uint8_t tempStatus = pxDataCmToCpu1->Payload.RxData.status;
        uint32_t seq1 = pxDataCmToCpu1->seqCount;
        if (seq0 == seq1) {
            // 정상적으로 읽었을 때만 실제 변수에 반영
            validSeq = tempSeq;
            validStatus = tempStatus;
        }
    }
    ```
  - **GS0 쓰기 (Seqlock 적용)**: 
    ```c
    pxDataCpu1ToCm->seqCount++; // 홀수 (쓰기 시작)
    pxDataCpu1ToCm->Payload.TxData.sineValue = sineValue;
    pxDataCpu1ToCm->Payload.TxData.adcTemperature = currentTemperatureC;
    pxDataCpu1ToCm->Payload.TxData.sequenceNum = ipcSeqNum;
    pxDataCpu1ToCm->seqCount++; // 짝수 (쓰기 완료)
    ```
  - **IPC 제거**: `IPC_sendCommand()` 호출 및 Busy 대기 로직을 완전히 삭제합니다.

---

### 비즈니스 로직 계층 (CSU) - CM

#### [MODIFY] CSU_Ethernet.c
- **내용**: 
  - `processReceivedEthernetPacket()` 함수 수정 (이더넷 수신 시):
    - 기존에 `sendIpcMessageToCPU1(...)`를 호출하여 CPU1에 IPC를 날리던 코드를 삭제합니다.
    - 대신 **GS1 쓰기 (Seqlock 적용)**: 
      ```c
      pxDataCmToCpu1->seqCount++; // 홀수 (쓰기 시작)
      pxDataCmToCpu1->Payload.RxData.seqNum = pPayload[12U];
      pxDataCmToCpu1->Payload.RxData.status = pPayload[13U];
      pxDataCmToCpu1->seqCount++; // 짝수 (쓰기 완료)
      ```
  - `buildAndSendUdpPacket()` 함수 수정 (이더넷 송신 시):
    - 패킷 조립 시 `xEthApp.txData`를 의존하지 않고 GS0 메모리를 Seqlock으로 읽어옵니다.
    - **GS0 읽기 (Seqlock 검증)**:
      ```c
      uint32_t seq0, seq1;
      do {
          seq0 = pxDataCpu1ToCm->seqCount;
          if (seq0 & 1U) continue; // 홀수면 CPU1이 쓰기 중이므로 대기
          
          // 데이터 읽기 복사
          tempSine = pxDataCpu1ToCm->Payload.TxData.sineValue;
          tempTemp = pxDataCpu1ToCm->Payload.TxData.adcTemperature;
          tempSeq  = pxDataCpu1ToCm->Payload.TxData.sequenceNum;
          
          seq1 = pxDataCpu1ToCm->seqCount;
      } while (seq0 != seq1); // 읽는 도중 CPU1이 업데이트했다면 다시 읽기
      
      // 읽어온 데이터를 기반으로 UDP Payload 구성
      ```

## Verification Plan

### Manual Verification
- **빌드 확인**: CPU1 및 CM 프로젝트를 각각 빌드하여 에러나 경고가 없는지 확인합니다.
- **메모리 브라우저 확인**: CCS 디버거에서 CPU1은 `0xD000`, CM은 `0x20014000`(GS0)를 띄워두고 값이 정상적으로 업데이트되는지 눈으로 확인합니다.
- **이더넷 통신 테스트**: PC 모니터링 프로그램을 통해 사인파 데이터와 온도가 끊김 없이 10ms (또는 2ms) 주기로 수신되는지 확인합니다.
- **성능 측정**: `MainControl_Isr`의 실행 시간이 줄어들었는지 GPIO 토글이나 CPU 타이머로 측정하여 검증합니다.
