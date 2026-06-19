# Research Report: EPWM1 ISR 미동작 및 GPIO 34번 토글 불발 원인 분석

## 1. 개요 (Overview)
CPU1 코어에서 100us 주기로 실행되도록 설계된 `MainControl_Isr` (EPWM1 기반) 인터럽트 루틴이 호출되지 않아, 해당 ISR 내부에서 제어하는 GPIO 34번(디버그용 점멸 LED)이 토글되지 않는 현상이 발생하고 있습니다. 
본 리서치는 C2000 MCU 하드웨어의 인터럽트, 타이머 동작 원리와 현 코드의 초기화 타이밍을 분석하여 이 문제의 원인을 규명하고 해결책을 제시합니다.

## 2. 코드 분석 및 원인 규명 (Root Cause Analysis)

문제의 원인은 크게 세 가지 타이밍 및 초기화 누락 요소가 복합적으로 작용한 결과입니다.

### 2.1. EPWM Event Trigger Flag(ETFLG) 미초기화 문제
- **현상**: `csu_Control.c`의 `Control_Init()`에서 `Interrupt_enable(INT_EPWM1);`을 통해 PIE 인터럽트를 활성화하기 전에, EPWM 모듈의 Event Trigger 인터럽트 플래그를 비워주지 않았습니다.
- **원인**: 디바이스 부팅 및 하드웨어 초기화 과정에서 EPWM 레지스터를 조작하다 보면 가짜(spurious) 이벤트가 발생하여 플래그가 1로 Set될 수 있습니다. 플래그가 이미 1인 상태에서는 PIE가 활성화되어도 하드웨어가 새로운 상승 에지(Edge) 펄스를 만들어내지 못하기 때문에, 첫 번째 인터럽트 진입이 영구히 봉쇄됩니다 (Deadlock 현상).
- **해결책**: 인터럽트를 활성화(enable)하기 직전에 반드시 `EPWM_clearEventTriggerInterruptFlag(EPWM_TIMER1_BASE);` 명령을 통해 플래그를 깨끗하게 강제 클리어해야 합니다.

### 2.2. 전역 인터럽트 활성화(`EINT`, `ERTM`) 시점의 오류
- **현상**: `hal_DspInit.c`의 `DSP_Initialization()` 함수 맨 마지막 부분에서 전역 인터럽트(`EINT`)와 디버그 인터럽트(`ERTM`)를 활성화하고 있습니다. 
- **원인**: `main_cpu1.c`의 부팅 시퀀스를 보면 `DSP_Initialization()` 호출이 끝난 **이후에** `Control_Init()`을 호출합니다. 즉, 문(Door)이 이미 활짝 열린(전역 인터럽트 ON) 상태에서 뒤늦게 문지기(인터럽트 벡터)를 등록하고 개별 인터럽트를 켜려는 형태입니다. 이는 C2000 하드웨어 구조상 레이스 컨디션을 유발할 수 있는 매우 위험한 안티 패턴입니다.
- **해결책**: `EINT; ERTM;` 호출 시점을 `main_cpu1.c`의 백그라운드 유휴 루프(`while(1)`) 진입 바로 직전으로 대이동(Shift)하여, 모든 시스템 인터럽트 등록이 100% 끝난 안전한 시점에 일제히 가동되도록 해야 합니다.

### 2.3. TBCLKSYNC (타임베이스 클럭 동기화) 관리 누락
- **현상**: `hal_Epwm.c`의 `Initial_EpwmTimer()`에서 EPWM1의 주기와 동작 모드 등을 세팅할 때, 전체 EPWM 카운터를 일시 정지시키는 `TBCLKSYNC` 제어가 누락되었습니다.
- **원인**: `Device_init()`에서 이미 `TBCLKSYNC`가 1로 설정되어 모든 클럭이 도는 상태인데, 이 상태에서 카운터 주기(`PRD`)를 수정하거나 카운터(`TBCTR`)를 강제로 0으로 밀어버리면 타이머 모듈 내부가 엉키거나 다른 PWM 채널과의 동기화가 완전히 깨져버립니다.
- **해결책**: EPWM 타이머를 설정하기 직전 `SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);`로 클럭을 멈추고 설정을 모두 마친 후, 다시 `SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);`를 호출해 모든 타이머가 깔끔하게 0부터 동시 출발하게 만들어야 합니다.

## 3. 구현 및 수정 계획 (Implementation Plan)

### 3.1. `main_cpu1.c` 수정
- 파일 최상단 수정 이력에 `EINT`, `ERTM` 이동 관련 내역 추가
- `while(1)` 메인 유휴 루프 진입 직전 위치로 `EINT;` 및 `ERTM;` 추가

### 3.2. `hal_DspInit.c` 수정
- 파일 최상단 수정 이력에 `EINT`, `ERTM` 이동 관련 내역 추가
- `DSP_Initialization()` 함수 맨 끝단에 존재하는 `EINT;` 및 `ERTM;` 삭제 (main_cpu1.c로 이관)

### 3.3. `csu_Control.c` 수정
- 파일 최상단 수정 이력에 인터럽트 플래그 강제 클리어 추가 내역 기재
- `Control_Init()` 함수 내부, `Interrupt_enable(INT_EPWM1);` 호출 바로 윗줄에 `EPWM_clearEventTriggerInterruptFlag(EPWM_TIMER1_BASE);` 삽입

### 3.4. `hal_Epwm.c` 수정
- 파일 최상단 수정 이력에 TBCLKSYNC 동기화 로직 추가 내역 기재
- `Initial_EpwmTimer()` 함수의 EPWM1 타이머 관련 설정 구간 앞/뒤로 `SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);` 와 `SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);` 추가

## 4. 예상 효과 및 부작용 검토 (Expected Results & Side Effects)
- **효과**: EPWM1 인터럽트가 100us 주기로 정상 진입하며, GPIO 34가 500ms 간격으로 정상 점멸하게 됩니다. 또한 CM 코어와의 IPC 통신 및 ADC 갱신 사이클도 완벽하게 살아납니다.
- **부작용 방어**: `EINT` 이동으로 인해 타 인터럽트(Timer0, SCI 등) 역시 초기화 과정 중 불필요하게 튀는 증상이 함께 방지되어 시스템 부팅 안정성이 극대화됩니다. `TBCLKSYNC` 처리 역시 EPWM8, EPWM9 등 이미 초기화된 타이머들과 동시에 출발선(0)에서 시작하도록 만들어 주어 완벽한 동기화를 보장합니다.
