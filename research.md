# CPU1 - CM 간 공유 메모리 리서치 보고서

## 1. 개요
현재 모터 제어 ISR 내에서 발생한 변수들을 이더넷을 통해 전송하기 위해, CPU1에서 데이터를 쓰고 `IPC_sendCommand()`를 호출하여 CM 코어에 인터럽트를 발생시키는 구조입니다. 이로 인한 오버헤드를 줄이고자 순수하게 공유 메모리(Shared Memory) 읽기/쓰기 방식을 통한 폴링(Polling) 구조로의 전환을 검토합니다.

## 2. MSGRAM vs GSRAM 비교 및 추천

F28388D 디바이스에는 코어 간 데이터를 공유할 수 있는 두 가지 메모리가 있습니다: **MSGRAM(Message RAM)**과 **GSRAM(Global Shared RAM)**.

| 특징 | MSGRAM (Message RAM) | GSRAM (Global Shared RAM) |
| --- | --- | --- |
| **용도** | IPC 통신 및 소규모 데이터 교환을 위한 전용 메모리 | 대규모 데이터 버퍼링 및 범용 메모리 |
| **개수 및 크기** | CPU1TOCM (1KB), CMTOCPU1 (1KB) 등 코어간 방향별 할당 | GS0 ~ GS15 (각 8KB), 총 128KB |
| **마스터십 제어** | **하드웨어적으로 방향 고정** (설정 불필요). <br>CPU1TOCM의 경우 CPU1은 쓰기/읽기, CM은 읽기 전용. | **CPU1에서 레지스터로 마스터 할당 필요**. <br>기본적으로 CPU1 소유이며, 특정 GS 블록(예: GS1)을 CM에 넘겨줄 수 있음. |
| **주소 변환** | 코어별 뷰(View) 주소가 다름 (CPU1: 0x39000, CM: 0x20080000) | 코어별 뷰(View) 주소가 다름 (CPU1: 0xD000, CM: 0x20014000) |

### 💡 어떤 것이 더 좋을까요?
- **현재 상황 (소규모 구조체 유지 시)**: 현재 `stIpcDataPacket` 정도의 작은 구조체만 사용한다면, 마스터십 관리가 필요없고 하드웨어적으로 접근 권한이 안전하게 보호되는 **MSGRAM**이 더 유리할 수 있습니다. (이미 `pxIpcCpu1ToCm` 포인터가 MSGRAM에 맵핑되어 있습니다.)
- **대용량 데이터 필요 시 (오실로스코프 파형 등)**: 향후 이더넷으로 보내야 할 데이터가 많아지거나 대형 배열을 사용한다면 크기가 1KB로 제한된 MSGRAM 대신 **GSRAM (GS0, GS1)**을 사용하는 것이 필수적입니다.
- **결론**: 확장성을 고려하여 메모리 용량이 충분한 **GSRAM (GS0, GS1)**에 전역 변수 구조체를 할당하고, 폴링(Polling) 방식으로 읽어가는 방식을 강력히 추천합니다.

## 3. GS0 / GS1을 활용한 양방향 공유 메모리 설계 (예시)

GS0는 CPU1 -> CM, GS1은 CM -> CPU1으로 할당하여 인터럽트 없이 데이터를 교환하는 설계 방법입니다.

### (1) 마스터십 설정 (CPU1)
GS0는 기본적으로 CPU1 소유이므로 그대로 두고, GS1의 마스터십만 CM 코어로 위임합니다.
```c
// hal_Ipc_cpu1.c 내 초기화 함수
MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS1, MEMCFG_GSRAMMASTER_CM); 
```

### (2) 코어별 메모리 주소 매핑
```c
// csu_Ipc_cpu1.h / csu_Ipc_cm.h 등에 공통 매크로 정의
#define GS0_CPU_ADDR  0x0000D000U // CPU1 관점 GS0
#define GS0_CM_ADDR   0x20014000U // CM 관점 GS0

#define GS1_CPU_ADDR  0x0000E000U // CPU1 관점 GS1
#define GS1_CM_ADDR   0x20016000U // CM 관점 GS1
```

### (3) 전역 변수 구조체 선언 및 활용
현재 프로젝트에서 IPC로 보내는 데이터를 메모리에 저장하고 구조체로 확인할 수 있는지에 대한 답변은 **"이미 가능하며, 약간의 주소 변경만 하면 됩니다"** 입니다.
```c
// ===== CPU1 코드 =====
// GS0 공간 (CPU1 쓰기, CM 읽기)
volatile stIpcDataPacket *pxDataCpu1ToCm = (volatile stIpcDataPacket *)GS0_CPU_ADDR;
// GS1 공간 (CM 쓰기, CPU1 읽기)
volatile stIpcDataPacket *pxDataCmToCpu1 = (volatile stIpcDataPacket *)GS1_CPU_ADDR;

// 모터 제어 ISR 내에서 (IPC_sendCommand 호출 제거)
pxDataCpu1ToCm->Payload.TxData.sineValue = sineValue;
pxDataCpu1ToCm->Payload.TxData.adcTemperature = currentTemperatureC;

// ===== CM 코드 =====
volatile stIpcDataPacket *pxDataCpu1ToCm = (volatile stIpcDataPacket *)GS0_CM_ADDR;
volatile stIpcDataPacket *pxDataCmToCpu1 = (volatile stIpcDataPacket *)GS1_CM_ADDR;

// 이더넷 전송 태스크 또는 Main Loop 내에서 직접 읽기 (인터럽트 불필요)
xEthApp.txData.SineVal = pxDataCpu1ToCm->Payload.TxData.sineValue;
```

## 4. 구조 전환 시 기대 효과 및 유의점
1. **IPC 오버헤드 완벽 제거**: `IPC_sendCommand`로 인한 잦은 인터럽트가 사라지므로, CPU1 모터 제어 루틴의 타이밍 지연(Jitter)이 사라지고 CM 코어의 부하도 획기적으로 줄어듭니다.
2. **동기화 문제 (Tearing)**: CPU1이 구조체를 업데이트하는 도중에 CM 코어가 절반만 읽어가는 데이터 불일치(Tearing) 문제가 발생할 수 있습니다. 하지만 이더넷 모니터링 수준의 데이터라면 무시할 수 있으며, 엄격한 동기가 필요하다면 구조체 맨 앞에 `SeqNum`이나 `UpdateFlag`를 두고 상태를 확인한 뒤 읽는 간단한 핸드쉐이킹을 추가하면 됩니다.
3. **가시성**: 디버거(CCS)의 Expressions 탭에서 `*pxDataCpu1ToCm`을 등록하면 전체 구조체 내용을 실시간 전역 변수처럼 쉽게 모니터링할 수 있습니다.
