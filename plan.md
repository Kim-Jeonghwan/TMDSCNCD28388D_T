# CM 및 CPU1 코어 CSU, HAL, main 코딩 규칙 및 구조 정비 계획서

## 1. 개요
프로젝트 내 **CSU(Control & Service Unit)** 및 **HAL(Hardware Abstraction Layer)** 아키텍처의 빌드 안전성과 코드 일관성을 높이기 위해, `GEMINI.md`에 명시된 코딩 표준을 벗어난 모든 미준수 사항(헤더 템플릿 누락, 소스 파일 내 매크로 선언, 부적절한 인클루드 의존성 등)을 정비하기 위한 계획서입니다.

---

## 2. Open Questions & User Review
> [!IMPORTANT]
> - **동작성(비즈니스 로직) 100% 보존**: 본 작업은 정적 구조 분석 결과에 따른 코드 정비 작업으로, 기존 프로그램의 컴파일 후 기능적 동작 로직은 완벽히 동일하게 유지됩니다.
> - **인코딩 및 한글 깨짐 방지**: 모든 소스 파일 수정 시 **UTF-8 인코딩**을 명시적으로 적용하고 검증하여, 기존에 작성된 상세한 한글 주석이 깨지지 않도록 보호합니다.
> - **헤더 주석 및 이력 업데이트**: 각 수정 파일의 최상단 템플릿의 `Last Updated`를 오늘 날짜(`2026. 06. 23. (코딩 규칙 준수 정비)`)로 갱신하고 `Modification History`에 이력을 명확히 남깁니다.
> - **에이전트 수칙 준수**: 에이전트는 사용자의 승인 지시(예: "승인합니다. 구현 시작해주세요.")가 떨어진 후 실제 코드 수정을 개시하며, 임의로 터미널 빌드를 실행하지 않고 정비가 끝나면 사용자에게 빌드 요청을 안내합니다.

---

## 3. 대상 파일 및 Proposed Changes

### 3.1. CPU1 CSU 계층 정비

#### [MODIFY] [csu_SciPc.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/CSU/csu_SciPc.h) / [csu_SciPc.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/CSU/csu_SciPc.c)
*   `csu_SciPc.h` 및 `csu_SciPc.c`에 누락된 `Modification History` 주석 블록을 추가합니다.
*   `csu_SciPc.c` 내부에 선언되어 있던 매크로 상수들을 `csu_SciPc.h`로 이동합니다.
    ```c
    #define SCI_PC_SOF     0x7Eu
    #define SCI_PC_EOT     0x0Du
    #define SCI_PC_MSG1    0x10u
    ```

#### [MODIFY] [csu_Led.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/CSU/csu_Led.h) / [csu_Led.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/CSU/csu_Led.c)
*   `csu_Led.c` 내부에 선언된 `#define LIMIT_TEMP_ERROR 45.0f`를 `csu_Led.h`로 이동합니다.

#### [MODIFY] [csu_Control.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/CSU/csu_Control.h) / [csu_Control.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/CSU/csu_Control.c)
*   `csu_Control.c` 내부에 선언된 `#define SINE_WAVE_STEP 0.000314159f`를 `csu_Control.h`로 이동합니다.

#### [MODIFY] [csu_Adc.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/CSU/csu_Adc.h)
*   소문자가 포함되어 있던 헤더 가드 매크로를 규칙에 맞추어 대문자화합니다.
    *   기존: `#ifndef csu_ADC_H` / `#define csu_Adc_H`
    *   변경: `#ifndef CSU_ADC_H` / `#define CSU_ADC_H`

---

### 3.2. CPU1 HAL 계층 정비
*   **[공통 사항]** 모든 CPU1 HAL 파일들의 최상단 표제 주석에 남아 있는 레거시 구조(`Copyright`, `Tracebility`, `Function List` 등)와 무의미한 탭 간격을 제거하고, `GEMINI.md`에 정의된 공통 표준 템플릿(8줄 규격)으로 완전히 일체화합니다.

#### [MODIFY] [hal_Timer.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Timer.h) / [hal_Timer.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Timer.c)
*   비어있던 `Programmer` 필드를 `Kim Jeonghwan`으로 채웁니다.

#### [MODIFY] [hal_Spi.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Spi.h) / [hal_Spi.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Spi.c)
*   비어있던 `Programmer` 필드를 `Kim Jeonghwan`으로 채웁니다.
*   `hal_Spi.c` 내부에 선언된 `#define ENCODER_SOMI_GPIC 51u` 및 `ENCODER_CLK_GPIC 52u`를 `hal_Spi.h`로 이동합니다.

#### [MODIFY] [hal_Sci.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Sci.h) / [hal_Sci.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Sci.c)
*   비어있던 `Programmer` 필드를 `Kim Jeonghwan`으로 채웁니다.
*   `hal_Sci.c` 내부에 선언된 하드웨어 GPIO 관련 매크로(`SCI_PC_GPIO_PIN_SCIA_RXD`, `SCI_PC_GPIO_PIN_SCIA_TXD`, `SCI_PC_GPIO_CFG_SCIA_RXD`, `SCI_PC_GPIO_CFG_SCIA_TXD`)를 `hal_Sci.h`로 이동합니다.

#### [MODIFY] [hal_Ramfuncs.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Ramfuncs.h) / [hal_Ramfuncs.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Ramfuncs.c)
*   비어있던 `Programmer` 필드를 `Kim Jeonghwan`으로 채웁니다.
*   `hal_Ramfuncs.c`에 누락된 `Modification History` 주석 블록을 보완합니다.

#### [MODIFY] [hal_DspInit.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_DspInit.h) / [hal_DspInit.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_DspInit.c)
*   `hal_DspInit.h` 내 비어있던 `Programmer`, `Description` 필드를 보완하고, 비어있던 `Modification History` 블록을 구성합니다.
*   `hal_DspInit.c` (L36)의 `#include "main_cpu1.h"`를 제거하고, 자기 자신의 헤더 파일인 `#include "hal_DspInit.h"`를 인클루드하도록 수정하여 인클루드 규칙을 준수합니다.

#### [MODIFY] [hal_Common.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Common.h) / [hal_Common.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Common.c)
*   비어있던 `Programmer`, `Description` 필드를 보완하고 `Modification History` 블록을 활성화합니다.

#### [MODIFY] [hal_Adc.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Adc.h) / [hal_Adc.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Adc.c)
*   `hal_Adc.h` 및 `hal_Adc.c` 내 비어있던 `Programmer`, `Description` 필드를 보완하고, `hal_Adc.c`에 누락된 `Modification History`를 추가합니다.
*   `hal_Adc.c`에 직접 선언된 매크로(`DEFAULT_MAVE_COUNT`, `DEFAULT_PWM_HZ`)를 `hal_Adc.h`로 이동합니다.
*   `hal_Adc.c` (L37)의 전역 변수 `uint16_t adcResult = 0u;` 선언을 `hal_Adc.h`에 `extern uint16_t adcResult;` 형태로 안전하게 선언하고, `hal_Adc.c`에서는 변수 정의만 유지합니다.

#### [MODIFY] [hal_Epwm.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Epwm.h)
*   누락된 `Modification History` 주석 블록을 추가합니다.

#### [MODIFY] [hal_Ipc_cpu1.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Ipc_cpu1.h) / [hal_Ipc_cpu1.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Ipc_cpu1.c)
*   `hal_Ipc_cpu1.h`에 누락된 `Modification History` 주석 블록을 보완합니다.
*   `hal_Ipc_cpu1.c` (L20)의 불필요하고 중복되는 `#include "csu_Ipc_cpu1.h"` 문장을 완전히 삭제합니다.

---

### 3.3. CM CSU 계층 정비

#### [MODIFY] [csu_Ethernet.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CM/CSU/csu_Ethernet.h) / [csu_Ethernet.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CM/CSU/csu_Ethernet.c)
*   `csu_Ethernet.h` 내 소문자가 섞여 있던 헤더 가드 매크로를 규칙에 맞추어 대문자화합니다.
    *   기존: `#ifndef csu_ETHERNET_H` / `#define csu_ETHERNET_H`
    *   변경: `#ifndef CSU_ETHERNET_H` / `#define CSU_ETHERNET_H`
*   `csu_Ethernet.c` 내에 정의되어 있던 이더넷 프로토콜 관련 매크로 상수 및 LED 제어 매크로(L45 ~ L91 영역 전체)를 `csu_Ethernet.h`로 일괄 이동하여 헤더 정의 규칙을 준수합니다.

---

### 3.4. CM HAL 계층 정비

#### [MODIFY] [hal_Ipc_cm.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CM/HAL/hal_Ipc_cm.h)
*   누락된 `Modification History` 주석 블록을 추가합니다.

#### [MODIFY] [hal_Timer.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CM/HAL/hal_Timer.h) / [hal_Timer.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CM/HAL/hal_Timer.c)
*   누락된 헤더 주석 내 `Version`, `Programmer` 필드 및 `Modification History` 블록을 보완합니다.
*   `hal_Timer.c` 내부 L10 ~ L13에 정의된 타이머 클럭 및 주기 상수 매크로들을 `hal_Timer.h`로 이동시킵니다.
    ```c
    #define CM_CLK_HZ          125000000U
    #define TIMER0_PERIOD_2MS  (CM_CLK_HZ / 500U)
    #define TIMER1_PERIOD_1MS  (CM_CLK_HZ / 1000U)
    #define TIMER2_PERIOD_1S   (CM_CLK_HZ / 1U)
    ```

---

## 4. 검증 계획
1. **[ ] UTF-8 인코딩 및 한글 보존성 검증**:
   * 각 수정 대상 파일의 변경 완료 직후 `view_file` 도구를 활용하여, 변경 전후의 한글 주석이 깨짐 없이 완전하게 UTF-8 형식으로 유지되고 있음을 즉각 확인합니다.
2. **[ ] 빌드 정합성 검증 요청**:
   * 소스 파일 정비 및 의존성 정리가 완료되면 사용자에게 안내하여, Code Composer Studio(CCS) IDE를 통해 CPU1 및 CM 프로젝트를 직접 컴파일 및 빌드하여 어떠한 컴파일 경고나 에러도 발생하지 않는지 검증하도록 요청합니다.
