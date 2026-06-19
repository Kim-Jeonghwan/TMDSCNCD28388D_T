# TMDSCNCD28388D_T LED 제어 로직 리팩토링 조사 보고서 (ATTLA_T 기준 동기화)

## 1. 개요
본 보고서는 `ATTLA_T` 프로젝트의 LED 제어 로직(`csu_Led.c`, `csu_Led.h`, `hal_DspInit.c`)을 분석하여, 현재 `TMDSCNCD28388D_T` 프로젝트의 CPU1 코어에 동일한 구조와 최적화 기법을 적용하기 위한 구체적인 구현 방안을 기술합니다.

## 2. ATTLA_T 프로젝트 분석 결과 (변경의 핵심)
`ATTLA_T` 프로젝트는 기존의 LED 제어 방식에서 다음과 같은 아키텍처 및 성능 최적화를 이루었습니다.

1. **비트필드 구조체 제거 (`stLed` in `csu_Led.h`)**
   - 기존의 `bool` 및 `:8u`, `:1u`와 같은 비트필드는 C2000 16비트 아키텍처에서 메모리 경계 초과 버그를 유발할 수 있습니다. 
   - 이를 해결하기 위해 구조체의 모든 멤버를 `uint16_t` 단일 자료형으로 통일하여 안정성을 확보했습니다.
2. **HAL 래퍼 함수 제거 (`csu_Led.c`)**
   - 불필요하게 `switch-case`문으로 분기되던 `HW_writeLedPin`과 `HW_toggleLedPin`을 제거했습니다.
   - 대신 SDK의 `GPIO_writePin`과 `GPIO_togglePin`에 구조체가 가진 `Index` 값을 직접 전달하여 호출 지연(Overhead)을 줄였습니다.
3. **GPIO 하드웨어 초기화의 계층 분리 (`hal_DspInit.c`)**
   - 상위 계층인 CSU(`csu_Led.c`)에 있던 하드웨어 핀 설정 로직(`initGpioDoutLed()`)을 제거했습니다.
   - 이를 HAL 계층인 `hal_DspInit.c`의 `Init_GpioDout()` 함수 내부로 이관하여 하드웨어 종속성을 완전히 HAL로 분리했습니다.
4. **자료형 통일 (`bool` -> `uint16_t`)**
   - 함수의 파라미터로 사용되던 `bool` 자료형을 모두 `uint16_t`로 변경하여 데이터 타입의 일관성을 맞추었습니다.

## 3. TMDSCNCD28388D_T 적용(구현) 계획

상기 분석을 바탕으로 `TMDSCNCD28388D_T` 프로젝트의 관련 파일을 다음과 같이 수정할 계획입니다.

### 3.1. `TMDSCNCD28388D_T_CPU1/CSU/csu_LED.h` 수정 방안
- `LED_OFF`, `LED_ON`, `LED_NONE`, `LED_TOGGLE` 매크로 값을 `false`/`true`에서 `0u`/`1u` (TMDSCNCD 특성에 맞게 Active High/Low 유지) 형태의 `uint16_t` 리터럴로 변경.
- `stLed` 구조체의 멤버 변수 자료형을 `bool`, 비트필드 방식에서 모두 `uint16_t`로 변경.
- `initGpioDoutLed()` 함수 선언 제거.
- `setLedStatus()`, `setLedModeToggle()` 함수의 `bool State` 파라미터를 `uint16_t State`로 변경.
- 헤더 상단에 표준 Modification History 업데이트.

### 3.2. `TMDSCNCD28388D_T_CPU1/CSU/csu_LED.c` 수정 방안
- `initGpioDoutLed()` 함수 구현 전체 제거 (해당 코드는 `hal_DspInit.c`로 이동).
- `HW_writeLedPin()`, `HW_toggleLedPin()` 정적 함수 구현 제거 및 선언 제거.
- `updateLedStatus()` 루프 내부의 핀 제어 로직을 `GPIO_writePin(pLed[i]->Index, pLed[i]->State)` 및 `GPIO_togglePin(pLed[i]->Index)`로 직접 호출하도록 수정.
- `setLedStatus()`, `setLedModeToggle()` 함수의 `bool` 파라미터를 `uint16_t`로 수정.
- 파일 상단에 표준 헤더 주석 및 Modification History 업데이트.

### 3.3. `TMDSCNCD28388D_T_CPU1/HAL/hal_DspInit.c` 수정 방안
- `Init_GpioDout()` 함수 내부에서 호출하던 `initGpioDoutLed();` 코드를 삭제.
- 대신 `csu_LED.c`에서 제거된 LED 핀(145, 146, 31~38)들의 설정 로직(`GPIO_setPinConfig`, `GPIO_setPadConfig`, `GPIO_setDirectionMode`, `GPIO_setMasterCore`)을 `Init_GpioDout()` 내부로 직접 이관하여 통합.
- 파일 상단에 표준 헤더 주석 및 Modification History 업데이트.

## 4. 결론 및 다음 단계
이상의 조사를 통해 `TMDSCNCD28388D_T` 프로젝트를 `ATTLA_T`와 완벽히 동일한 구조로 리팩토링할 준비가 완료되었습니다. 본 내용에 대해 확인 및 추가 코멘트(예: 파일명 소문자 통일 여부, LED Active State 확인 등)를 반영할 수 있습니다. 
승인이 떨어지면 즉시 `plan.md`를 작성하거나 구현에 착수할 수 있습니다.
