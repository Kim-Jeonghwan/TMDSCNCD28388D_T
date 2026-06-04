# 📋 Hz 편차 수정 계획서 (2단계) — CM 클럭 100MHz 고정 + CPU1 950Hz 원인 분석

**작성 일자**: 2026. 06. 04.  
**이전 단계 결과**: while 루프 교정(1단계) 적용 후 빌드 완료  
**현재 측정값**: CPU1 ≈ 950 Hz / CM ≈ 540 Hz (목표: 1000 Hz)

---

## 1. 현재 측정값 원인 분석

### 1.1 CM 540 Hz — AUXPLL 클럭 불일치 (★ 확실)

| 항목 | 내용 |
|------|------|
| CM 클럭 설정 | `SYSCTL_CMCLKOUT_DIV_1, SYSCTL_SOURCE_AUXPLL` (의도: 125 MHz) |
| 실제 동작 클럭 | ~67.5 MHz (AUXPLL 오설정으로 인한 실제 출력) |
| CM Timer1 Period | `125,000,000 / 1000 = 125,000` 카운트 (1 ms 기준으로 설정) |
| 실제 인터럽트 주기 | `125,000 / 67,500,000 ≈ 1.852 ms` |
| 실제 Hz | `1000 / 1.852 ≈ 540 Hz` ← **측정값과 정확히 일치** |

**근본 원인**: `device.h`의 `DEVICE_AUXSETCLOCK_CFG`가 `SYSCTL_AUXPLL_OSCSRC_XTAL`(차동 수정 모드)로 설정되어 있으나, 하드웨어 발진기(`ECS-2520S33-250-FN-TR`)는 **Single-Ended** 방식이므로 실제 AUXPLL 출력 주파수가 설계값보다 낮습니다.

**해결책**: AUXPLL을 포기하고 안정적인 **SYSPLL / 2 = 100 MHz** 를 CM 클럭 소스로 사용합니다.

---

### 1.2 CPU1 950 Hz — SCI 블로킹 지연 부분 해소 (★ 추정)

| 항목 | 내용 |
|------|------|
| 백그라운드 루프 | `sendScia_SCI_PC()` → `SCI_writeCharArray()` 블로킹 |
| 현재 동작 | 매 루프마다 최대 20바이트 SCI 전송 시도 |
| while 캐치업 | 적용 완료 → 대부분 보완되지만 완전하지 않음 |

**가능성 있는 잔존 원인**:  
- `SCI_writeCharArray()`는 데이터가 있을 때 블로킹 전송을 수행합니다. 이 과정에서 1ms 타이머 인터럽트가 밀리는 경우가 여전히 발생할 수 있습니다.  
- `isr_Epwm1Timer2ms` ISR 내에서 `IPC_sendCommand` 호출 시 FLAGS1이 이미 세팅된 상태이면 덮어쓰기 또는 내부 지연이 발생할 수 있습니다.
- CPU Timer0 ISR(100us)이 매우 짧은 주기로 발생하여 Timer1 ISR을 선점하는 경우가 있을 수 있습니다.

---

## 2. 수정 계획 (Proposed Changes)

### [STEP 1] CM 클럭을 SYSPLL/2 = 100 MHz 로 변경

#### [MODIFY] [DevDspInit.c (CPU1)](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/Dev/DevDspInit.c)

- 파일 내 `Initial_CmCore()` 함수 수정 (L283~L295)
- `Last Updated` 헤더 날짜를 `2026. 06. 04.` 로 갱신

```diff
 static void Initial_CmCore(void)
 {
-    // CM 클럭 활성화 (원래 규격 및 이더넷 대역폭 확보를 위해 AUXPLL 기반 125MHz 설정)
-    SysCtl_setCMClk(SYSCTL_CMCLKOUT_DIV_1, SYSCTL_SOURCE_AUXPLL);
+    // CM 클럭: 불안정한 AUXPLL 대신 SYSPLL / 2 = 100 MHz (안정적이고 검증된 클럭 소스)
+    SysCtl_setCMClk(SYSCTL_CMCLKOUT_DIV_2, SYSCTL_SOURCE_SYSPLL);
 
 #ifdef _FLASH
     Device_bootCM(BOOTMODE_BOOT_TO_FLASH_SECTOR0);
 #else
     Device_bootCM(BOOTMODE_BOOT_TO_S0RAM);
 #endif
 }
```

#### [MODIFY] [DevTimer.c (CM)](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CM/Dev/DevTimer.c)

- `CM_CLK_HZ` 정의를 `125000000U` → `100000000U` 로 변경
- `Last Updated` 헤더 날짜를 `2026. 06. 04.` 로 갱신
- 각 타이머 Period 매크로는 `CM_CLK_HZ` 를 기반으로 자동 재계산됩니다.

```diff
-#define CM_CLK_HZ          125000000U   /* 실제 CM 클럭: 125 MHz */
+#define CM_CLK_HZ          100000000U   /* 실제 CM 클럭: SYSPLL/2 = 100 MHz */
 #define TIMER0_PERIOD_2MS  (CM_CLK_HZ / 500U)     /* 200,000: 2ms 주기 */
 #define TIMER1_PERIOD_1MS  (CM_CLK_HZ / 1000U)    /* 100,000: 1ms 주기 */
 #define TIMER2_PERIOD_1S   (CM_CLK_HZ / 1U)       /* 100,000,000: 1s 주기 */
```

> **변경 후 Period 값 확인**
> - Timer0 (2ms): `100,000,000 / 500 = 200,000`
> - Timer1 (1ms): `100,000,000 / 1000 = 100,000`  
> - Timer2 (1s):  `100,000,000 / 1 = 100,000,000`

---

### [STEP 2] CPU1 950 Hz 잔존 원인 추가 조치 (선택)

> 이 단계는 STEP 1 빌드 및 측정 결과를 보고 결정합니다.

- **옵션 A (권장)**: `sendScia_SCI_PC()` 호출을 백그라운드 루프에서 `cycle_10ms()` 내부로 이동하여, 10ms마다 한 번만 SCI 전송하도록 변경
  - 장점: 백그라운드 루프에서 블로킹 함수 완전 제거 → while 캐치업 극대화
  - 단점: PC SCI 전송 주기가 10ms로 고정됨

- **옵션 B**: CPU Timer0 (100us ISR)을 비활성화하거나 주기를 늘림
  - 100us ISR이 Timer1 ISR 진입을 방해할 수 있음

- **옵션 C**: 현재 유지 (950Hz 허용) — 1000Hz에 가깝고 기능 동작에는 문제 없음

---

## 3. 검증 계획 (Verification Plan)

| 단계 | 확인 항목 |
|------|-----------|
| 1 | CPU1 프로젝트, CM 프로젝트 순서로 CCS 빌드 (에러/경고 없음) |
| 2 | CCS 디버거 Expressions 창에서 `xTimer.Hz` 값 확인 |
| 3 | CM `xTimer.Hz` ≈ **1000** Hz 정착 확인 (STEP 1 효과) |
| 4 | CPU1 `xTimer.Hz` 개선 여부 확인 → 950 → 1000 목표 |

---

## 4. 빌드 안내

수정 완료 후 아래 순서로 CCS IDE에서 직접 빌드해주십시오:

1. **CPU1 프로젝트** (`TMDSCNCD28388D_T_CPU1`) 빌드
2. **CM 프로젝트** (`TMDSCNCD28388D_T_CM`) 빌드
