# 📋 IPC / Ethernet UDP 통신 장애 수정 계획서

**작성 일자**: 2026. 06. 05.  
**이전 단계 결과**: SOP/EOP 플래그 수정 완료, 진단 카운터 삽입 완료  
**현재 상태**: TX 전부 실패 (`g_uiTxOkCnt=0`), IPC ISR 미호출 (`g_uiIpcIsr1Cnt=0`)  
**근거 문서**: [research.md §20](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/research.md) 참조

---

## 1. 장애 원인 요약 및 수정 우선순위

| 순위 | 원인 | 영향도 | 상태 |
|------|------|--------|------|
| 1 | **SOP/EOP 플래그 누락** — `Ethernet_sendPacket` 즉시 거부 | TX 전면 실패 | ✅ 수정 완료 |
| 2 | **IPC ISR Race Condition** — EPWM ISR이 CM보다 먼저 FLAG1 점유 | IPC 데이터 전달 불가 | ⬜ 수정 예정 |
| 3 | **DevEthernet.c 미사용 `s_xTxPktDesc` 제거** — 코드 정리 | 동작 영향 없음 | ⬜ 수정 예정 |

---

## 2. 수정 계획 (Proposed Changes)

### [STEP 1] CM `Initial_IPC()` — 잔류 FLAG1 강제 ACK (★ 핵심 수정)

> **목적**: CM이 IPC_INT1 핸들러를 등록한 직후, CPU1이 이미 설정해놓은 FLAG1의 잔류 상태를 강제 클리어하여 다음 IPC_sendCommand에서 새로운 rising edge가 발생하도록 보장합니다.

#### [MODIFY] [DevIPC.c (CM)](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CM/Dev/DevIPC.c)

- `Initial_IPC()` 함수에 잔류 FLAG1 ACK 로직 추가
- `Last Updated` 헤더 날짜를 `2026. 06. 05.` 로 갱신

```diff
 void Initial_IPC(void)
 {
-    // IPC_init()은 CPU1의 초기화 상태를 보호하기 위해 호출을 생략합니다.
-
     // 1. CPU1으로부터 수신받을 인터럽트 등록
     IPC_registerInterrupt(IPC_CM_L_CPU1_R, IPC_INT1, isrIpcFromCPU1);
 
+    // 2. [핵심 수정] 등록 전에 CPU1이 이미 설정한 FLAG1 잔류 플래그 강제 ACK
+    //    (EPWM ISR Race Condition으로 인해 CM 등록 전에 FLAG1이 SET될 수 있음)
+    //    ACK 후 다음 IPC_sendCommand에서 새로운 rising edge → ISR 정상 트리거
+    if (IPC_isFlagBusyRtoL(IPC_CM_L_CPU1_R, IPC_FLAG1))
+    {
+        IPC_ackFlagRtoL(IPC_CM_L_CPU1_R, IPC_FLAG1);
+    }
+
-    // 2. CPU1 코어와 동기화 수행
+    // 3. CPU1 코어와 동기화 수행
     IPC_sync(IPC_CM_L_CPU1_R, IPC_FLAG31);
 }
```

**왜 이 위치인가?**:
- `IPC_registerInterrupt` → ISR 핸들러 등록 + NVIC 인터럽트 활성화
- 바로 직후에 잔류 FLAG1을 ACK하면, FLAG1이 LOW로 떨어짐
- 이후 CPU1이 다음 2ms에 `IPC_sendCommand(FLAG1)`을 호출하면 LOW→HIGH 전이 발생
- NVIC가 rising edge를 감지 → ISR 정상 트리거!

---

### [STEP 2] CPU1 `sendEthDataToCM()` — 방어적 busy 체크 추가 (안전성 강화)

> **목적**: ISR 컨텍스트에서 호출되는 `sendEthDataToCM`이 FLAG1 busy 상태를 명시적으로 스킵하도록 하여, SDK `IPC_sendCommand`의 반환값 무시 문제를 방어합니다.

#### [MODIFY] [DevIPC.c (CPU1)](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/Dev/DevIPC.c)

- `sendEthDataToCM()` 함수에 FLAG1 busy 체크 guard 추가
- `Last Updated` 헤더 날짜를 `2026. 06. 05.` 로 갱신

```diff
 void sendEthDataToCM(uint16_t dspTemp, uint8_t seqNum, uint8_t status)
 {
+    /* CM이 이전 메시지를 아직 ACK하지 않았으면 스킵 (2ms 후 재시도) */
+    if (IPC_isFlagBusyLtoR(IPC_CPU1_L_CM_R, IPC_FLAG1))
+    {
+        return;
+    }
+
     uint32_t uiAddr = (uint32_t)dspTemp;
     uint32_t uiData = ((uint32_t)status << 8U) | (uint32_t)seqNum;
     IPC_sendCommand(IPC_CPU1_L_CM_R, IPC_FLAG1, IPC_ADDR_CORRECTION_DISABLE,
                     (uint32_t)IPC_CMD_CPU1_ETH_TX_DATA, uiAddr, uiData);
 }
```

**설계 이유**:
- SDK의 `IPC_sendCommand` 내부에도 busy 체크가 있지만, 반환값을 무시하면 **명령 레지스터에 데이터를 쓰지 않은 채** 함수가 종료되므로, 상위 레이어에서 명시적으로 guard하는 것이 안전합니다.
- ISR 컨텍스트에서 블로킹 대기는 불가하므로, busy이면 즉시 return하고 다음 2ms 주기에서 재시도합니다.
- **주의**: TI Clang에서 지역변수 선언 전 return문은 C99 이상에서 허용되지만, 코딩 규칙 상 함수 선두에 모든 변수 선언 후 로직이 필요할 경우 구조를 조정할 수 있습니다.

---

### [STEP 3] DevEthernet.c — 미사용 `s_xTxPktDesc` 제거 (코드 정리)

> **목적**: CSU_Ethernet.c에 이미 동일 이름의 `static` 변수가 있으므로, DevEthernet.c의 것은 불필요합니다.

#### [MODIFY] [DevEthernet.c (CM)](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CM/Dev/DevEthernet.c)

- L51: `static Ethernet_Pkt_Desc s_xTxPktDesc;` 삭제
- L128: `(void)memset(&s_xTxPktDesc, 0, sizeof(s_xTxPktDesc));` 삭제
- `Last Updated` 헤더 날짜를 `2026. 06. 05.` 로 갱신

```diff
 static uint8_t           s_ucRxBuf[ETH_RX_NUM_PKT_DESC][ETH_RX_BUF_SIZE];
 static Ethernet_Pkt_Desc s_xRxPktDesc[ETH_RX_NUM_PKT_DESC];
-static Ethernet_Pkt_Desc s_xTxPktDesc;
 
 ...
 
     initRxDescriptors();
     (void)memset(g_ucTxBuf, 0, ETH_TX_BUF_SIZE);
-    (void)memset(&s_xTxPktDesc, 0, sizeof(s_xTxPktDesc));
```

---

## 3. 옵션: 진단 카운터 제거 여부

현재 `CSU_Ethernet.c`와 `DevIPC.c (CM)`에 진단용 `volatile` 카운터 변수가 추가되어 있습니다:
- `g_uiTxOkCnt`, `g_uiTxFailCnt` (CSU_Ethernet.c)
- `g_uiIpcIsr1Cnt` (DevIPC.c)

**옵션 A (권장)**: 통신이 정상 동작함이 확인될 때까지 유지 → 향후 제거
**옵션 B**: 이번 수정과 함께 제거

> 현재는 디버깅 과정이므로 **옵션 A (유지)**를 권장합니다.

---

## 4. 수정 파일 요약

| 프로젝트 | 파일 | 수정 내용 |
|----------|------|-----------|
| **CM** | `Dev/DevIPC.c` | `Initial_IPC()`에 잔류 FLAG1 ACK 로직 추가 |
| **CPU1** | `Dev/DevIPC.c` | `sendEthDataToCM()`에 FLAG1 busy 체크 guard 추가 |
| **CM** | `Dev/DevEthernet.c` | 미사용 `s_xTxPktDesc` 변수 및 memset 제거 |

---

## 5. 검증 계획 (Verification Plan)

| 단계 | 확인 항목 |
|------|-----------|
| 1 | CPU1 프로젝트 빌드 (에러/경고 없음) |
| 2 | CM 프로젝트 빌드 (에러/경고 없음) |
| 3 | CCS Expressions 창에서 `g_uiIpcIsr1Cnt` → **0보다 크게 증가** 확인 |
| 4 | CCS Expressions 창에서 `g_xEthTxData.SeqNum` → **증가** 확인 |
| 5 | CCS Expressions 창에서 `g_uiTxOkCnt` → **증가**, `g_uiTxFailCnt` → **0 유지** 확인 |
| 6 | PC 네트워크 상태 → **수신 바이트 증가** 확인 |
| 7 | PC 앱에서 UDP 패킷 정상 수신 확인 |

---

## 6. 빌드 안내

수정 완료 후 아래 순서로 CCS IDE에서 직접 빌드해주십시오:

1. **CPU1 프로젝트** (`TMDSCNCD28388D_T_CPU1`) 빌드
2. **CM 프로젝트** (`TMDSCNCD28388D_T_CM`) 빌드
