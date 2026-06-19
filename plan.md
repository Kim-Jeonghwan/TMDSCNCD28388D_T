# 아키텍처 표준화 및 리팩토링 계획 (Implementation Plan)

## 1. 개요
프로젝트 전체(CPU1, CM)의 CSU, HAL, main 소스 코드를 스캔 및 분석하여 최신 아키텍처 규정(이름 규칙, 헤더 포함 의존성, 매크로 위치, CM 코어 전용 규칙 등)에 위배되는 사항들을 모두 식별하였습니다. 이를 기반으로 일관성 있는 펌웨어 아키텍처로 리팩토링하기 위한 종합 계획입니다.

> [!IMPORTANT]
> **User Review Required**
> 1. 모듈 접두사(`csu_`, `hal_`)를 변수명 및 함수명에서 일괄 제거할 경우, 전역 영역에서 이름 충돌(Name Collision)이 발생할 가능성이 있는지 확인해 주세요. (예: `hal_Adc_read` -> `Adc_read` 등)
> 2. `CSU_Ethernet.c` 등 일부 파일에서 상당히 많은 매크로가 `.c` 내부에 정의되어 있습니다. 이를 모두 해당 `.h` 파일로 이동시키는 것을 승인해 주시기 바랍니다.

## 2. Proposed Changes

### Component 1: File Naming Convention (파일 네이밍 규칙 수정)
- 모듈 및 파일명에만 소문자 접두사(`csu_`, `hal_`)를 사용해야 하므로 대문자로 시작하는 파일명을 수정합니다.
#### [MODIFY] `CSU_Ethernet.c` -> `csu_Ethernet.c`
#### [MODIFY] `CSU_Ethernet.h` -> `csu_Ethernet.h`

---

### Component 2: Header Dependency & Include Rules (헤더 인클루드 규칙 수정)
- **.c 소스 파일 규칙**: 자신의 이름과 동일한 헤더 파일 단 하나만 포함해야 합니다.
- **.h 헤더 파일 규칙**: 오직 중심 허브인 `main_cpu1.h` 또는 `main_cm.h`만을 포함해야 합니다.

#### [MODIFY] CPU1 `.c` / `.h` 파일 일괄 수정
- `hal_DspInit.c`: `#include "main_cpu1.h"` 등 불필요한 인클루드 제거 및 `#include "hal_DspInit.h"`만 남김.
- 그 외 발견된 `.c` 및 `.h` 파일들의 `#include` 구문 정리 (CPU1 CSU/HAL 전역).

#### [MODIFY] CM `.c` / `.h` 파일 일괄 수정
- `csu_Ethernet.c` 및 `hal_Timer.c` 등의 파일에서 `#include` 구문 정리.

---

### Component 3: Macros & Constants Move (매크로/상수 선언 헤더로 이동)
- 모든 `#define` 및 전역 변수 선언을 `.c`에서 `.h` 파일로 이동시킵니다.

#### [MODIFY] CPU1: 매크로가 포함된 `.c` 파일들
- `csu_Control.c`, `csu_Led.c`, `csu_SciPc.c`
- `hal_Adc.c`, `hal_Sci.c`, `hal_Spi.c`
- 위 파일들의 내부 `#define` 매크로들을 각각의 헤더 파일로 이동.

#### [MODIFY] CM: 매크로가 포함된 `.c` 파일들
- `csu_Ethernet.c` (약 19개의 매크로 존재)
- `hal_Timer.c` (약 4개의 매크로 존재)
- 내부 `#define` 매크로들을 각각의 헤더 파일로 이동.

---

### Component 4: Prefix Elimination (구조체, 변수, 함수명 접두어 제거)
- 구조체, 변수명, 함수명에 `csu_` 또는 `hal_` 접두어가 사용된 부분을 찾아 순수한 이름으로 변경합니다.

#### [MODIFY] CPU1 코드 영역 접두어 제거
- `hal_Adc.c`, `csu_Adc.c` 등에서 교차 사용된 변수/함수명 (예: `csu_Adc`, `hal_Adc`, `hal_Epwm`, `csu_Led` 등을 포함하는 이름) 제거 처리.
- `main_cpu1.h`의 구조체 선언부 점검 및 `xHalEth`, `xCsuEth` 등의 이름 수정.

#### [MODIFY] CM 코드 영역 접두어 제거
- `csu_Ethernet.c/h`, `hal_Ipc_cm.c/h` 등에서 사용된 변수/함수명 수정.

---

### Component 5: CM Core ARM Cortex-M4 Rules (CM 코어 특수 규칙 반영)
- ARM 환경에 맞지 않는 C28x DSP 전용 코드를 수정합니다.

#### [MODIFY] `main_cm.c` (CM)
- DSP 스타일의 `__asm(" NOP");` 명령어를 CMSIS 표준 함수인 `__NOP();` 로 변경.
- 데이터 타입(예: `int`, `char`) 사용 점검 및 `uint16_t`, `uint32_t` 등 고정 크기 타입으로 강제.

---

### Component 6: Documentation & Standards (주석 및 품질 규격)
#### [MODIFY] 모든 수정 대상 CSU/HAL 및 main 파일
- 파일 최상단 Header Comment Auto-Update (`Version`, `Last Updated`).
- `Modification History` 항목에 수정 내역 추가 (예: `2026. 06. 19. - 아키텍처 규칙에 따른 헤더 및 매크로 수정`).

## 3. 질문 및 확인 사항 (Open Questions)
> [!WARNING]
> 현재 `main_cpu1.h`와 `main_cm.h` 내에 모든 하위 헤더들(`hal_*.h`, `csu_*.h`)이 포함(include)되어 있으며, 이들은 상호 의존성을 갖게 됩니다. 이로 인해 특정 모듈에서 `csu_`, `hal_` 접두사를 일괄 제거하게 되면 다른 모듈과 이름이 겹치는 문제가 발생할 수 있습니다. 예를 들어, `Timer_init` 이라는 이름이 CSU와 HAL 양쪽에서 선언될 위험이 있습니다. 이 경우 모듈명을 어떻게 구분하실지 방향성 승인이 필요합니다.

## 4. 검증 계획 (Verification Plan)
1. **Automated Analysis**:
   - 파이썬 스크립트를 재실행하여 아키텍처 규정 위반이 없는지 100% 점검.
2. **Build Verification**:
   - IDE (CCS Theia)에서 CPU1 및 CM 프로젝트를 각각 `gmake` (빌드)하여 컴파일 에러나 헤더 순환 참조 에러가 없는지 사용자에게 확인 요청.
3. **Static Test Compliance**:
   - 무기체계 소프트웨어 코딩규칙 (Cyclomatic Complexity, Call Levels 등) 위반 여부 점검.
