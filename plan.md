# TMDSCNCD28388D_T LED 제어 구현 계획서 (plan.md)

## 1. 목표 (Goal)
TMDSCNCD28388D_T 프로젝트의 CPU1 코어 LED 제어 방식을 ATTLA_T 프로젝트와 동일한 구조로 최적화하고, 요구사항에 맞게 GPIO 핀을 재설정 및 동작 주기를 연동합니다.

## 2. 세부 요구사항 (Requirements)
1. **사용 핀 변경**: 기존 `GPIO145` 대신 **`GPIO31`**을 RUN LED로 사용.
2. **에러 LED 삭제**: 기존 `GPIO146` 에러 LED 및 관련 코드 모두 삭제.
3. **EPWM1 연동 LED 추가**: **`GPIO34`**를 ATTLA_T와 동일하게 `EPWM1` 주기(100us)마다 점멸하도록 ISR 내부에 반영.
4. **ATTLA_T 구조 최적화 적용**:
   - `csu_LED.h`: 비트필드 구조체 제거, 자료형 통일(`uint16_t`).
   - `csu_LED.c`: 불필요한 `switch-case` 기반 HAL 래퍼 함수 제거, SDK 함수 직접 호출.
   - `hal_DspInit.c`: GPIO 초기화 로직을 CSU에서 HAL 계층으로 이관.

---

## 3. 파일별 수정 계획 (Proposed Changes)

### 3.1. `TMDSCNCD28388D_T_CPU1/CSU/csu_LED.h`
- **매크로 정의 변경**: 
  - `LED_OFF` (1u), `LED_ON` (0u) 및 `LED_TOGGLE` (1u), `LED_NONE` (0u)로 Active Low 방식에 맞추어 자료형 변경 (`bool` -> `uint16_t`).
- **`eLed` 열거형 변경**:
  - `eLED_RUN = 31u`만 남기고 `eLED_ERROR`, `eLED_01` ~ `eLED_08` 모두 삭제.
- **`stLed` 구조체 개선**:
  - 기존 비트필드(`:8u`, `:1u`) 방식을 제거하고, 모든 멤버를 `uint16_t` 단일 자료형으로 통일하여 메모리 경계 초과 버그 방지.
- **`stLedStatus` 구조체 변경**:
  - `ledRun` 멤버만 유지.
- **함수 선언 변경**:
  - `initGpioDoutLed(void)` 선언 삭제.
  - `setLedStatus()`, `setLedModeToggle()`의 파라미터를 `bool`에서 `uint16_t`로 변경.

### 3.2. `TMDSCNCD28388D_T_CPU1/CSU/csu_LED.c`
- **불필요한 함수 제거**:
  - `initGpioDoutLed()` 본문 삭제 (HAL로 이관).
  - `HW_writeLedPin()`, `HW_toggleLedPin()` 정적 함수 삭제.
- **`Initial_LED()` 수정**:
  - `xLed.ledRun` (GPIO 31)만 초기화하도록 남기고, 나머지 에러 및 배열 LED 초기화 코드 삭제.
- **`updateLedStatus()` 수정**:
  - 내부 온도 센서 에러 기준(`xAdc.currentTemperatureC >= LIMIT_TEMP_ERROR`)으로 에러 LED를 점등하던 로직 삭제.
  - 배열 루프의 `HW_*` 래퍼를 제거하고, `GPIO_writePin()` 및 `GPIO_togglePin()`을 직접 호출하도록 리팩토링.
- **파라미터 타입 변경**:
  - `setLedStatus()`, `setLedModeToggle()`의 `State` 파라미터를 `uint16_t`로 일괄 수정.

### 3.3. `TMDSCNCD28388D_T_CPU1/HAL/hal_DspInit.c`
- **`Init_GpioDout()` 함수 수정**:
  - 기존 `initGpioDoutLed();` 호출 코드 삭제.
  - `GPIO31` (RUN LED, 기존 LED 01) 및 `GPIO34` (EPWM1 연동 임시 LED, 기존 LED 04)의 초기화 로직을 직접 작성 (출력, 푸시풀 모드 등).
  - 나머지 미사용 LED(GPIO 145, 146 등) 초기화는 제거.

### 3.4. `TMDSCNCD28388D_T_CPU1/HAL/hal_EpwmTimer.c`
- **`isr_Epwm1Timer100us()` 함수 수정**:
  - 해당 ISR 내부 루틴 맨 마지막 부분에 `GPIO_togglePin(34U);` 코드를 추가.
  - 100us마다 트리거되는 인터럽트 주기에 맞추어 `GPIO34` 핀이 상태를 반전(Toggle)하게 되어 오실로스코프로 ISR 동작 주기를 검증할 수 있도록 조치.

## 4. 검증 계획 (Verification Plan)
- 변경된 파일에 대해 문법 에러가 없는지 사용자에게 빌드를 요청.
- 빌드 완료 후 장비 구동 시, GPIO31이 정상적으로 점멸하는지 확인.
- 오실로스코프를 통해 GPIO34의 점멸 주기가 100us (10kHz)로 출력되는지 실측 확인.

---

> [!IMPORTANT]
> 계획서 내용을 검토해 주시기 바랍니다. 승인해 주시면 즉시 코드 변경을 시작하겠습니다.
