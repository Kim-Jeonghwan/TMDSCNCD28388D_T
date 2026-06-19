# EPWM1 ISR 미동작 수정 및 시스템 부팅 안정화 계획 (Implementation Plan)

## 1. 개요 (Overview)
CPU1의 `MainControl_Isr`가 실행되지 않아 GPIO 34번 토글 등 주요 100us 제어 루프가 멈추는 현상을 해결하기 위한 구현 계획입니다.
본 계획은 앞서 진행된 `research.md`의 분석 결과를 바탕으로, 소스 코드의 무단 축약 없이 정확히 필요한 위치만 국소적으로 수정하여 시스템의 안정성을 극대화하는 것을 목표로 합니다.

## 2. 사용자 검토 필요 사항 (User Review Required)
> [!IMPORTANT]
> - 펌웨어 제어 코드(C2000)를 직접 수정하는 작업이므로, 사용자님의 **명시적인 구현 승인**이 필요합니다.
> - `main_cpu1.c`에서 전역 인터럽트(`EINT; ERTM;`) 위치를 백그라운드 루프 진입 직전으로 옮김으로써, 전체 시스템의 인터럽트 시작 시점이 깔끔하게 동기화됩니다. 이 방식에 동의하시는지 확인 바랍니다.
> - 이 문서(`plan.md`)에 추가적인 제약 사항이나 수정 방향을 코멘트로 남겨주시면 모두 반영하겠습니다.

## 3. 수정 진행 현황 (Progress)
- [x] **`main_cpu1.c` 수정 완료**: 전역 인터럽트(`EINT`, `ERTM`)를 메인 유휴 루프 직전으로 이동
- [x] **`hal_DspInit.c` 수정 완료**: 조기 활성화된 전역 인터럽트 명령 제거
- [x] **`csu_Control.c` 수정 완료**: PIE 인터럽트 활성화 전 EPWM 타이머 플래그 클리어 적용
- [x] **`hal_Epwm.c` 수정 완료**: EPWM 타이머 초기화 전후 TBCLKSYNC 동기화 잠금/해제 적용

### 3.1. CPU1 Core Initialization
#### [MODIFY] [main_cpu1.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/main_cpu1.c)
- **변경 목적**: 안전한 전역 인터럽트(`EINT`) 가동 시점 확보
- **수정 내용**: 
  - `while (1u)` 유휴 루프에 진입하기 바로 직전에 `EINT;` 와 `ERTM;` 코드를 추가합니다.
  - 헤더 주석의 `Modification History`에 날짜와 함께 해당 내역을 추가합니다.

#### [MODIFY] [hal_DspInit.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_DspInit.c)
- **변경 목적**: 위험한 인터럽트 조기 가동 방지
- **수정 내용**: 
  - `DSP_Initialization()` 함수 맨 마지막에 위치한 `EINT;`, `ERTM;` 명령어를 **제거**합니다. (main_cpu1.c로 이관됨)
  - 헤더 주석의 `Modification History`에 날짜와 함께 해당 내역을 추가합니다.

---

### 3.2. EPWM & Interrupt Control
#### [MODIFY] [csu_Control.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/CSU/csu_Control.c)
- **변경 목적**: 죽어버린 EPWM1 ISR 활성화 (Deadlock 방지)
- **수정 내용**: 
  - `Control_Init()` 함수 내부의 `Interrupt_enable(INT_EPWM1);` 호출 바로 윗줄에 `EPWM_clearEventTriggerInterruptFlag(EPWM_TIMER1_BASE);` 를 추가하여, 초기화 시 발생했을 수 있는 가짜 펜딩 플래그를 말끔히 지웁니다.
  - 헤더 주석의 `Modification History`에 변경 내역을 추가합니다.

#### [MODIFY] [hal_Epwm.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/HAL/hal_Epwm.c)
- **변경 목적**: EPWM 타이머 카운터 및 클럭 동기화
- **수정 내용**: 
  - `Initial_EpwmTimer()` 함수 초반에 `SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);` 를 삽입하여 전체 타이머 클럭을 잠시 멈춥니다.
  - 타이머 레지스터 세팅(`EPWM_setClockPrescaler` 등)이 모두 끝난 함수 최하단에서 `SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);` 를 다시 호출하여 클럭 동기화를 재개시킵니다.
  - 헤더 주석의 `Modification History`에 변경 내역을 추가합니다.

## 4. 검증 계획 (Verification Plan)
### Manual Verification
- 컴파일 및 빌드가 정상적으로 완료되는지 확인 요청.
- 펌웨어를 장비에 다운로드 한 후, GPIO 34번(디버그 LED)이 500ms(0.5초) 주기로 안정적으로 점멸하는지 육안 확인.
- CM 코어와의 이더넷/IPC 통신(사인파 및 ADC 데이터 전송)이 원활하게 복구되었는지 확인.
