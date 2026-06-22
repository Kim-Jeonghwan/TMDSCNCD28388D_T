# CM 및 CPU1 코어 CSU, HAL, main 코딩 규칙 및 구조 분석 보고서

## 1. 개요 및 목적
본 보고서는 프로젝트 내 **CSU (Control & Service Unit)** 계층, **HAL (Hardware Abstraction Layer)** 계층, 그리고 **Main** 관련 소스코드와 헤더 파일을 대상으로 `GEMINI.md`에 정의된 **코딩 규칙 및 구조(CSU/HAL 아키텍처 규칙)**의 준수 여부를 종합적으로 전수 조사한 결과입니다.

---

## 2. 조사 대상 디렉토리 및 파일
1. **CPU1 코어 (`TMDSCNCD28388D_T_CPU1`)**
   - `CSU/` 하위 소스 및 헤더 파일 (총 12개)
   - `HAL/` 하위 소스 및 헤더 파일 (총 18개)
   - 메인 소스 및 헤더 파일 (`main_cpu1.c`, `main_cpu1.h`)
2. **CM 코어 (`TMDSCNCD28388D_T_CM`)**
   - `CSU/` 하위 소스 및 헤더 파일 (총 4개)
   - `HAL/` 하위 소스 및 헤더 파일 (총 6개)
   - 메인 소스 및 헤더 파일 (`main_cm.c`, `main_cm.h`)

---

## 3. 코딩 규칙 미준수 사항 상세

### 3.1. 헤더 주석 템플릿 및 수정 이력(Modification History) 누락
헤더 주석 템플릿(Nexcom Co., Ltd. 로고, 파일명, 버전, 설명, 작성자 `Kim Jeonghwan`, 최종 수정일)의 필수 항목이 누락되었거나, 헤더 주석 직후에 유지되어야 하는 `Modification History` 블록이 누락된 파일들의 목록입니다.

#### ① 작성자(Programmer) 및 설명(Description) 필드가 비어있거나 누락된 파일
*   **CPU1 HAL**:
    *   `hal_Timer.h` / `hal_Timer.c` (Programmer가 비어있음)
    *   `hal_Spi.h` / `hal_Spi.c` (Programmer가 비어있음)
    *   `hal_Sci.h` / `hal_Sci.c` (Programmer가 비어있음)
    *   `hal_Ramfuncs.h` / `hal_Ramfuncs.c` (Programmer가 비어있음)
    *   `hal_DspInit.h` (Programmer, Description이 비어있음)
    *   `hal_Common.h` / `hal_Common.c` (Programmer, Description이 비어있음)
    *   `hal_Adc.h` (Programmer, Description이 비어있음)
    *   `hal_Adc.c` (Programmer가 비어있음)
*   **CM HAL**:
    *   `hal_Timer.h` / `hal_Timer.c` (헤더 템플릿 내 Programmer, Version 필드 자체가 아예 누락됨)

#### ② 수정 이력(Modification History) 블록 자체가 누락된 파일
*   **CPU1 CSU**:
    *   `csu_SciPc.h` / `csu_SciPc.c` (Modification History 주석 블록 없음)
*   **CPU1 HAL**:
    *   `hal_Adc.c` (Modification History 주석 블록 없음)
    *   `hal_Epwm.h` (Modification History 주석 블록 없음)
    *   `hal_Ipc_cpu1.h` (Modification History 주석 블록 없음)
    *   `hal_Ramfuncs.c` (Modification History 주석 블록 없음)
    *   `hal_Common.h` / `hal_Common.c` (Modification History가 비어있음)
    *   `hal_DspInit.h` (Modification History가 비어있음)
*   **CM HAL**:
    *   `hal_Ipc_cm.h` (Modification History 주석 블록 없음)
    *   `hal_Timer.h` / `hal_Timer.c` (Modification History 주석 블록 없음)

---

### 3.2. 매크로 상수 (`#define`) 소스 파일(.c) 내부 선언 규칙 위반
> **[규칙]** 모든 매크로 상수(`#define`), 전역 변수, 스케일 팩터 등의 선언은 `.c` 소스 파일 내부가 아닌, 반드시 해당 모듈의 헤더 파일(`.h`)에 작성해야 합니다.

소스 파일 내부에 선언되어 있어 헤더 파일(`.h`)로 이동해야 하는 매크로 목록입니다:

| 파일 경로 | 위반 매크로 선언 내용 |
| :--- | :--- |
| `TMDSCNCD28388D_T_CPU1/CSU/csu_SciPc.c` | `#define SCI_PC_SOF 0x7Eu`<br>`#define SCI_PC_EOT 0x0Du`<br>`#define SCI_PC_MSG1 0x10u` |
| `TMDSCNCD28388D_T_CPU1/CSU/csu_Led.c` | `#define LIMIT_TEMP_ERROR 45.0f` |
| `TMDSCNCD28388D_T_CPU1/CSU/csu_Control.c` | `#define SINE_WAVE_STEP 0.000314159f` |
| `TMDSCNCD28388D_T_CPU1/HAL/hal_Spi.c` | `#define ENCODER_SOMI_GPIC 51u`<br>`#define ENCODER_CLK_GPIC 52u` |
| `TMDSCNCD28388D_T_CPU1/HAL/hal_Sci.c` | `#define SCI_PC_GPIO_PIN_SCIA_RXD 28u`<br>`#define SCI_PC_GPIO_PIN_SCIA_TXD 29u`<br>`#define SCI_PC_GPIO_CFG_SCIA_RXD GPIO_28_SCIA_RX`<br>`#define SCI_PC_GPIO_CFG_SCIA_TXD GPIO_29_SCIA_TX` |
| `TMDSCNCD28388D_T_CPU1/HAL/hal_Adc.c` | `#define DEFAULT_MAVE_COUNT 100u`<br>`#define DEFAULT_PWM_HZ 100000u` |
| `TMDSCNCD28388D_T_CM/CSU/csu_Ethernet.c` | `ETH_TX_NUM_PKT_DESC`, `ETH_HDR_DST_OFFSET`, `ETH_HDR_SRC_OFFSET`, `ETH_HDR_TYPE_OFFSET`, `ETH_HDR_SIZE`, `IP_HDR_OFFSET`, `IP_HDR_VER_IHL`, `IP_HDR_DSCP`, `IP_TTL`, `IP_PROTO_UDP`, `IP_HDR_SIZE`, `UDP_HDR_OFFSET`, `UDP_HDR_SIZE`, `PAYLOAD_OFFSET`, `TX_REFLECT_FRAME_SIZE`, `TX_ACK_FRAME_SIZE`, `MIN_RX_FRAME_SIZE`, `ETH_LED_PIN`, `ETH_LED_ON()` 등 (L45 ~ L91 영역 전체) |
| `TMDSCNCD28388D_T_CM/HAL/hal_Timer.c` | `#define CM_CLK_HZ 125000000U`<br>`#define TIMER0_PERIOD_2MS (CM_CLK_HZ / 500U)`<br>`#define TIMER1_PERIOD_1MS (CM_CLK_HZ / 1000U)`<br>`#define TIMER2_PERIOD_1S (CM_CLK_HZ / 1U)` |

---

### 3.3. 헤더 인클루드 규칙 위반
> **[규칙]** 각 개별 소스 파일(`.c`)은 자신의 이름과 동일한 헤더 파일(`.h`) 단 하나만 `#include` 해야 합니다.

*   `TMDSCNCD28388D_T_CPU1/HAL/hal_Ipc_cpu1.c` (L20):
    *   위반 내용: `#include "csu_Ipc_cpu1.h"`를 중복 인클루드함.
    *   조치: `hal_Ipc_cpu1.h`가 `main_cpu1.h`를 임포트하고 있으며, `main_cpu1.h`에 이미 `csu_Ipc_cpu1.h`가 정의되어 있어 중복이자 위반입니다. (해당 행 삭제 필요)
*   `TMDSCNCD28388D_T_CPU1/HAL/hal_DspInit.c` (L36):
    *   위반 내용: `#include "main_cpu1.h"`를 직접 인클루드함.
    *   조치: 자신의 헤더인 `#include "hal_DspInit.h"`만 인클루드하도록 변경해야 함. (`hal_DspInit.h`에서 `main_cpu1.h`를 인클루드하고 있으므로 의존성은 동일하게 해결됨)

---

### 3.4. 전역 변수 선언 위치 규칙 위반
> **[규칙]** 모든 매크로 상수, 전역 변수 등의 선언은 `.c` 내부가 아닌 반드시 헤더 파일(`.h`)에 작성해야 합니다.

*   `TMDSCNCD28388D_T_CPU1/HAL/hal_Adc.c` (L37):
    *   위반 내용: `uint16_t adcResult = 0u;` 전역 변수가 소스 파일 내에 직접 정의되어 있습니다.
    *   조치: `hal_Adc.h`에 `extern uint16_t adcResult;`를 선언하고, 소스 파일에서는 정의만 유지해야 합니다.

---

### 3.5. 소문자 헤더 가드 매크로 규칙 위반
> **[규칙]** 헤더 가드 매크로는 대문자 `CSU_` 및 `HAL_`을 사용해야 합니다.

*   `TMDSCNCD28388D_T_CPU1/CSU/csu_Adc.h` (L18~19):
    *   `#ifndef csu_ADC_H` / `#define csu_Adc_H` ➡️ `#ifndef CSU_ADC_H` / `#define CSU_ADC_H`로 수정 필요.
*   `TMDSCNCD28388D_T_CM/CSU/csu_Ethernet.h` (L19~20):
    *   `#ifndef csu_ETHERNET_H` / `#define csu_ETHERNET_H` ➡️ `#ifndef CSU_ETHERNET_H` / `#define CSU_ETHERNET_H`로 수정 필요.

---

## 4. 기타 검증 및 분석 (규칙 준수 항목)
1.  **CM 코어 (ARM Cortex-M4F) 규칙 검증**:
    *   `EALLOW` / `EDIS` 키워드 혼용 없음 (0건).
    *   인터럽트 정의 시 C28x 전용 `interrupt` / `__interrupt` 키워드 미사용 (0건, NVIC 표준 핸들러 형식 준수).
    *   CM 전용 SysCtl API (`CM_SysCtl_...`) 완벽하게 준수하여 사용.
    *   모든 변수에 고정 크기 데이터 타입 (`uint8_t`, `uint16_t`, `uint32_t` 등)을 철저히 준수하여 선언.
2.  **CSU 및 HAL 명명 규칙**:
    *   소문자 `csu_` 및 `hal_` 접두어가 함수명, 변수명, 구조체명 등에 오용된 사례는 단 한 건도 없음. 파일명에만 명확하게 접두어를 사용하여 계층 분리 보존.
3.  **한글 주석 및 가독성**:
    *   모든 custom 파일 내 주석은 한국어로 일관되게 적용되었으며, 중괄호 `{ }` 포맷 역시 다중 줄 가독성 구조를 잘 유지하고 있음.

---

## 5. 향후 계획 및 조치 방안 (제안)
상기 미준수 사항들은 빌드 안전성과 코드 일관성, 그리고 규칙 강제성을 높이기 위해 모두 표준에 맞게 정비되어야 합니다.
사용자분께서 **구현 및 수정 계획 수립을 승인해 주시면**, 수정 작업을 위한 세부적인 `plan.md` 파일을 생성하여 단계적 수정 시퀀스를 제안하고 정비에 착수하겠습니다.
