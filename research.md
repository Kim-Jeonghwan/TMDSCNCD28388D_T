# CSU 및 HAL 계층의 csu_ / hal_ 접두사 사용 조사 보고서

## 1. 개요 및 목적
본 보고서는 프로젝트 내 **CSU (Control & Service Unit)** 계층과 **HAL (Hardware Abstraction Layer)** 계층의 소스코드 및 헤더 파일을 대상으로 `csu_` 또는 `hal_` 접두사가 함수명, 변수명, 구조체명 등에 규칙을 위반하여 사용되었는지 전수 조사한 결과입니다.

**[명명 규칙 수칙 준수 검증]**
- **CSU**: 파일명/모듈명에만 소문자 `csu_`를 사용해야 하며, 함수/변수/구조체명에는 절대 사용하지 않아야 함.
- **HAL**: 파일명/모듈명에만 소문자 `hal_`을 사용해야 하며, 함수/변수/구조체명에는 절대 사용하지 않아야 함. 헤더 가드 매크로는 대문자 `HAL_`을 사용해야 함.

---

## 2. 조사 대상 디렉토리
1. **CPU1 코어 (`TMDSCNCD28388D_T_CPU1`)**
   - `CSU/` 디렉토리 하위 모든 `.c` 및 `.h` 파일
   - `HAL/` 디렉토리 하위 모든 `.c` 및 `.h` 파일
2. **CM 코어 (`TMDSCNCD28388D_T_CM`)**
   - `CSU/` 디렉토리 하위 모든 `.c` 및 `.h` 파일
   - `HAL/` 디렉토리 하위 모든 `.c` 및 `.h` 파일
3. 메인 소스코드 (`main_cpu1.c/h`, `main_cm.c/h`)의 호출 부분

---

## 3. 조사 결과 요약

### 3.1. 함수명, 변수명, 구조체명 검증 결과
- **CPU1 및 CM 전체 코드 내에서 함수명, 변수명, 구조체명 앞에 `csu_` 또는 `hal_` (대소문자 불문) 접두사가 사용된 사례는 단 한 건도 발견되지 않았습니다.**
- CSU 계층의 함수는 `Control_Init()`, `sendScia_SCI_PC()`, `updateLedStatus()`와 같이 대문자로 시작하는 카멜케이스 등을 사용하고 있으며, HAL 계층의 함수 역시 `DSP_Initialization()`, `Initial_IPC()` 등 접두사가 붙지 않은 표준적인 명명법을 고수하고 있습니다.
- 이는 프로젝트 아키텍처 상의 명명 규칙(함수/변수/구조체에 접두사 사용 금지 수칙)이 완벽히 준수되고 있음을 나타냅니다.

### 3.2. 예외 및 특이사항 (소문자 헤더 가드 매크로 검출)
기능적 함수나 변수는 아니지만, 헤더 파일의 중복 포함을 방지하기 위한 **헤더 가드(Header Guard) 매크로**에서 규칙에 어긋나게 소문자 `csu_`가 포함된 매크로명이 일부 발견되었습니다.

| 파일 경로 | 기존 선언 내용 (소문자 포함) | 규칙 부합 선언 (제안) |
| :--- | :--- | :--- |
| `TMDSCNCD28388D_T_CPU1/CSU/csu_Adc.h` | `#ifndef csu_ADC_H`<br>`#define csu_Adc_H` | `#ifndef CSU_ADC_H`<br>`#define CSU_ADC_H` |
| `TMDSCNCD28388D_T_CM/CSU/csu_Ethernet.h` | `#ifndef csu_ETHERNET_H`<br>`#define csu_ETHERNET_H` | `#ifndef CSU_ETHERNET_H`<br>`#define CSU_ETHERNET_H` |

---

## 4. 파일별 세부 검색 이력 (csu_ 및 hal_ 매칭 내역)

### 4.1. 소문자 csu_ 접두사 검색 결과 (Exact Case)
주로 물리 파일명을 참조하는 `#include` 구문 및 이력 관련 주석에만 사용되고 있으며, 코드 자체의 심볼(Symbol)로 쓰인 것은 헤더 가드를 제외하고 존재하지 않습니다.
- **CPU1 코어**:
  - `main_cpu1.h` L52~57: `#include "csu_SciPc.h"`, `#include "csu_Led.h"`, `#include "csu_Adc.h"`, `#include "csu_Epwm.h"`, `#include "csu_Ipc_cpu1.h"`, `#include "csu_Control.h"`
  - `HAL/hal_DspInit.c` L21: `* 2026. 06. 19. - csu_Led.c에서 분리된 GPIO 31 및 34 초기화 로직을 Init_GpioDout()에 직접 통합` (주석)
  - `HAL/hal_Epwm.c` L13, L29: 주석 내 파일명 지칭
  - `HAL/hal_Ipc_cpu1.c` L20: `#include "csu_Ipc_cpu1.h"`
  - `HAL/hal_Adc.c` L37: `uint16_t adcResult = 0u; // (csu_Adc.c에서 참조)` (주석)
  - `CSU/csu_Adc.h` L18~19: `#ifndef csu_ADC_H` / `#define csu_ADC_H` **(소문자 매크로)**
- **CM 코어**:
  - `main_cm.h` L39~40: `#include "csu_Ipc_cm.h"`, `#include "csu_Ethernet.h"`
  - `HAL/hal_Ipc_cm.c` L54: `csu_Ipc_cm 레이어로 처리 요청을 넘기고` (주석)
  - `HAL/hal_Ethernet.h` L43: `(csu_Ethernet.c 등에서 사용)` (주석)
  - `CSU/csu_Ethernet.h` L19~20: `#ifndef csu_ETHERNET_H` / `#define csu_ETHERNET_H` **(소문자 매크로)**

### 4.2. 소문자 hal_ 접두사 검색 결과 (Exact Case)
마찬가지로 `#include "hal_xxx.h"` 형태의 지시문과 주석에서만 검출되었습니다.
- **CPU1 코어**:
  - `main_cpu1.h` L42~50: `#include "hal_Common.h"`, `#include "hal_DspInit.h"`, `#include "hal_Sci.h"`, `#include "hal_Spi.h"`, `#include "hal_Timer.h"`, `#include "hal_Adc.h"`, `#include "hal_Ipc_cpu1.h"`, `#include "hal_Epwm.h"`, `#include "hal_Ramfuncs.h"`
  - `CSU/csu_Control.c` L15: `* 2026. 06. 19. - hal_Epwm.c에서 100us 타이머 ISR 및 제어 로직 이전` (주석)
  - `CSU/csu_Adc.c` L22: `// hal_Adc.c에 선언된 실시간 온도 센서 원시 결과` (주석)
- **CM 코어**:
  - `main_cm.h` L34~36: `#include "hal_Ipc_cm.h"`, `#include "hal_Ethernet.h"`, `#include "hal_Timer.h"`
  - `HAL/hal_Ethernet.c` L20: `(CPU1 hal_DspInit.c 에서 GPIO 핀 MUX 설정)` (주석)

### 4.3. 대문자 CSU_ 및 HAL_ 검색 결과 (Exact Case)
대문자로 작성된 `CSU_` 및 `HAL_`의 경우, **100% 헤더 가드 매크로 명칭으로만 정상 사용**되고 있음이 검증되었습니다.
- 예: `#ifndef HAL_TIMER_H`, `#ifndef CSU_SCIPC_H` 등

---

## 5. 향후 조치 제안
- 현재 명명 수칙 위반 상태인 `csu_Adc.h`와 `csu_Ethernet.h`의 소문자 헤더 가드 매크로명을 대문자로 변경하는 것을 제안합니다.
- 이에 대한 구체적인 수정 작업을 진행하려면 사용자의 승인이 필요합니다.
