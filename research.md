# 파일명 및 모듈명 리팩토링 리서치 보고서

## 1. 개요
사용자의 요청에 따라 `CPU1` 및 `CM` 프로젝트 내의 주요 `CSU` 및 `HAL` 계층 파일들의 이름을 명명 규칙에 맞게 변경하고, 관련된 헤더 파일 포함(`#include`), 헤더 가드, 파일 내부 주석 등에 대한 전면적인 리팩토링을 수행하기 위한 조사를 완료하였습니다.

## 2. 변경 대상 파일 목록 (Rename Target)

### 2.1 CPU1 프로젝트 (TMDSCNCD28388D_T_CPU1)
| 기존 파일명 | 변경할 파일명 |
| --- | --- |
| `CSU/csu_EPWM.c` / `.h` | `CSU/csu_Epwm.c` / `.h` |
| `CSU/csu_IPC.c` / `.h` | `CSU/csu_Ipc_cpu1.c` / `.h` |
| `CSU/csu_LED.c` / `.h` | `CSU/csu_Led.c` / `.h` |
| `CSU/csu_SCI_PC.c` / `.h` | `CSU/csu_SciPc.c` / `.h` |
| `HAL/hal_EpwmTimer.c` / `.h` | `HAL/hal_Epwm.c` / `.h` |
| `HAL/hal_IPC.c` / `.h` | `HAL/hal_Ipc_cpu1.c` / `.h` |

### 2.2 CM 프로젝트 (TMDSCNCD28388D_T_CM)
| 기존 파일명 | 변경할 파일명 |
| --- | --- |
| `CSU/csu_IPC.c` / `.h` | `CSU/csu_Ipc_cm.c` / `.h` |
| `HAL/hal_IPC.c` / `.h` | `HAL/hal_Ipc_cm.c` / `.h` |


## 3. 코드 내부 수정 대상 상세

### 3.1 헤더 인클루드 (`#include`) 수정 (main.h 및 개별 c 파일)
*   **CPU1 `main.h`**
    *   `#include "csu_EPWM.h"` ➔ `#include "csu_Epwm.h"`
    *   `#include "csu_IPC.h"` ➔ `#include "csu_Ipc_cpu1.h"`
    *   `#include "csu_LED.h"` ➔ `#include "csu_Led.h"`
    *   `#include "csu_SCI_PC.h"` ➔ `#include "csu_SciPc.h"`
    *   `#include "hal_EpwmTimer.h"` ➔ `#include "hal_Epwm.h"`
    *   `#include "hal_IPC.h"` ➔ `#include "hal_Ipc_cpu1.h"`
*   **CM `main.h`**
    *   `#include "csu_IPC.h"` ➔ `#include "csu_Ipc_cm.h"`
    *   `#include "hal_IPC.h"` ➔ `#include "hal_Ipc_cm.h"`
*   **개별 소스 코드 파일** 내부에서도 자기 자신의 헤더 참조 시 새로운 이름으로 포함하도록 변경해야 합니다 (예: `csu_Epwm.c` 파일 내 `#include "csu_Epwm.h"`).
*   **CPU1 `HAL/hal_Ipc_cpu1.c`**: 기존의 `#include "csu_IPC.h"`를 새로운 이름인 `#include "csu_Ipc_cpu1.h"`로 수정해야 합니다.

### 3.2 헤더 가드 매크로 (`#ifndef` / `#define`) 표준화
현재 일부 파일의 헤더 가드가 소문자를 포함하고 있거나 기존 이름을 따르고 있습니다. 명명 규칙("헤더 가드 등의 매크로는 대문자 HAL_ 등을 사용합니다.")에 맞추어 아래와 같이 대문자로 수정합니다.

*   **CPU1**
    *   `csu_Epwm.h`: `csu_EPWM_H` ➔ `CSU_EPWM_H`
    *   `csu_Ipc_cpu1.h`: `csu_IPC_H` ➔ `CSU_IPC_CPU1_H`
    *   `csu_Led.h`: `csu_LED_H` ➔ `CSU_LED_H`
    *   `csu_SciPc.h`: `csu_SCI_PC_H` ➔ `CSU_SCIPC_H`
    *   `hal_Epwm.h`: `HAL_EPWM_TIMER_H` ➔ `HAL_EPWM_H`
    *   `hal_Ipc_cpu1.h`: `HAL_IPC_H` ➔ `HAL_IPC_CPU1_H`
*   **CM**
    *   `csu_Ipc_cm.h`: `csu_IPC_H` ➔ `CSU_IPC_CM_H`
    *   `hal_Ipc_cm.h`: `HAL_IPC_H` ➔ `HAL_IPC_CM_H`

### 3.3 파일 상단 템플릿(주석) 갱신
모든 변경 대상 파일(총 16개 파일, .c 및 .h)의 최상단 주석 템플릿 영역에 기재된 `Filename` 항목을 새로운 파일명으로 갱신해야 합니다. 더불어, `Last Updated`에 변경 사유(예: "모듈 및 파일명 리팩토링") 및 수정 일자를 기록해야 합니다.

### 3.4 타 파일 내 주석 및 참조 텍스트 업데이트
다른 소스 파일에서 기존 파일명을 언급하며 설명하고 있는 주석 내용도 일관성을 위해 동기화가 필요합니다.
*   **CPU1 `hal_Epwm.c` (구 `hal_EpwmTimer.c`)**: `csu_SCI_PC.c 에서 ADC 읽어서` ➔ `csu_SciPc.c 에서 ADC 읽어서`
*   **CPU1 `hal_DspInit.c`**: `csu_LED.c에서 분리된` ➔ `csu_Led.c에서 분리된`
*   **CPU1 `csu_Ipc_cpu1.c` (구 `csu_IPC.c`)**: `csu_SCI_PC.c 등에서 참조` ➔ `csu_SciPc.c 등에서 참조`
*   **CPU1 `hal_Ipc_cpu1.c` (구 `hal_IPC.c`)**: `csu_IPC 레이어로 전달` ➔ `csu_Ipc_cpu1 레이어로 전달`
*   **CM `csu_Ethernet.h`, `csu_Ethernet.c`**: `csu_IPC.c 에서 갱신` ➔ `csu_Ipc_cm.c 에서 갱신`
*   **CM `hal_Ipc_cm.c` (구 `hal_IPC.c`)**: `csu_IPC 레이어로 처리 요청` ➔ `csu_Ipc_cm 레이어로 처리 요청`

## 4. 리서치 결론
프로젝트 전반에 걸친 변경 대상이므로 각 계층(CSU, HAL) 내부 뿐만 아니라 `main.h`와 타 모듈의 주석 참조 내역까지 모두 함께 치환(Replace)해야 합니다. 변수 및 함수 명명 규칙 상 이미 `Initial_Epwm7a`나 `recvSciPcMessage`와 같이 함수명은 prefix 규칙에 구속받지 않는 형태로 작성되어 있으므로 파일 이름과 `#include` 구문, 그리고 헤더 가드만 전면 교체하면 시스템 빌드 및 동작에 지장이 없을 것으로 판단됩니다.
