# 시스템 아키텍처 및 통신 구조 리팩토링 계획서 (Implementation Plan)

본 계획서는 `TMDSCNCD28388D_T` 프로젝트의 이더넷 통신 구조를 폴링 방식에서 인터럽트 방식으로 변경하고, 100us 주기의 메인 루프에서 ADC 처리와 IPC 연동을 수행하기 위한 구체적인 구현 단계를 명세합니다.

> [!IMPORTANT]
> **사용자 검토 및 승인 요청 (User Review Required)**
> 본 계획서를 검토하시고 승인(Plan Approval)해 주시면, 작성된 Phase 1부터 실제 코드 구현 작업을 진행하겠습니다. 요구사항 추가나 로직 변경을 원하시면 이 `plan.md` 파일에 메모나 주석을 직접 달아주시면 반영하겠습니다.

---

## 1. 개요 및 구현 목표 (Goals)
1. **코드 품질 향상**: `ATTLA_T` 프로젝트의 우수한 주석 방식(버전, 수정 이력, Doxygen 포맷)과 구조체 기반 상태 관리 방식 도입.
2. **실시간 제어 최적화 (CPU1)**: 100us(10kHz) 주기 `EPWM1` 인터럽트를 메인 제어 루프로 승격시키고, 기존의 ADC 처리 로직을 이 ISR 내부로 이동.
3. **코어 간 고속 통신 (IPC)**: CPU1의 EPWM1 루프에서 테스트용 사인파(Sine wave)를 생성하고, ADC 데이터와 함께 구조체로 패킹하여 CM 코어로 IPC 전송.
4. **이더넷 인터럽트 응답 (CM)**: CM 코어에서 IPC 수신 인터럽트를 통해 데이터를 전역 버퍼에 저장하고, PC로부터 100ms 데이터 요청 패킷이 수신되면 W6100 Rx 인터럽트를 통해 즉시 응답 전송.
5. **PC 모니터링 기능 (C#)**: 100ms 주기로 타겟 보드에 데이터를 요청하고, 수신된 응답 패킷(사인파, ADC)을 실시간 차트(Graph)에 렌더링.

---

## 2. 세부 구현 단계 (Implementation Phases)

### Phase 1: 코드 스타일 및 주석 방식 도입 (Code Style & Structure Integration)
- **대상**: CPU1 및 CM 코어의 `CSU`, `HAL`, `main.c/h`
- **내용**:
  - 파일 최상단 헤더 주석에 `Version`, `Programmer`, `Modification History` 템플릿 적용.
  - 모든 주요 함수에 Doxygen 스타일 주석(`@function`, `@brief`, `@param`, `@return`, `@remark`) 적용.
  - 흩어진 전역 변수를 구조체(`stAdcState xAdc`, `stSysCtrl xSysCtrl`)로 캡슐화.
  - 매직 넘버(스케일 팩터 등)를 직관적인 상수로 치환.

### Phase 2: CPU1 코어 - 100us EPWM 인터럽트 루프 구성 및 ADC 연동
- **대상**: `TMDSCNCD28388D_T_CPU1` (특히 `hal_EpwmTimer.c`, `csu_EPWM.c`, `csu_Adc.c`)
- **내용**:
  - `EPWM1` 주기를 100us (10kHz)로 설정하고 타겟 인터럽트(`INT_EPWM1`)와 연동.
  - EPWM1 ISR 내부에 기존 `10ms` 주기이던 ADC 측정 데이터 변환 로직(`updateAdcData`)을 호출하도록 이동시켜 실시간성 확보.
  - EPWM1 ISR 내부에 수식(`sin()`)이나 LUT를 활용하여 테스트용 **사인파(Sine wave) 발생 로직** 구현.
  - C2000 전용 방식인 비트필드(예: `EPwm1Regs`, `AdcaRegs`) 및 `EALLOW`, `EDIS`, `__interrupt` 문법을 엄격히 준수.

### Phase 3: 코어 간 데이터 통신망 구축 (CPU1 -> CM IPC 전송)
- **대상**: CPU1의 `hal_IPC.c`, `csu_IPC.c`
- **내용**:
  - 사인파 데이터와 변환된 ADC 결과값을 담을 전용 IPC 송신 구조체(`stIpcTxToCm`)를 정의하고 공유 RAM 공간(GSRAM 또는 IPC Message RAM)에 할당.
  - EPWM1 ISR 내부에서 구조체 값을 갱신한 후, IPC 플래그를 Set하여 CM 코어로 IPC 통신 발생.
  - C28x 코어 특성(16-bit 주소 체계)에 맞추어 데이터 타입 크기 및 정렬 관리.

### Phase 4: CM 코어 - IPC 수신 인터럽트 및 데이터 버퍼링
- **대상**: `TMDSCNCD28388D_T_CM` (특히 `hal_IPC.c`, `csu_IPC.c`)
- **내용**:
  - CM 코어에서 발생하는 IPC 수신 인터럽트 처리기(ISR) 등록.
  - ARM Cortex-M4F(CM) 특성을 준수하여, `EALLOW`, `EDIS`, `__interrupt` 등의 DSP 전용 문법 사용 금지.
  - CM 전용 NVIC 기반으로 ISR을 등록하고, `CM_IPC_...` Driverlib API를 사용하여 플래그 클리어 처리.
  - 수신된 데이터 구조체를 읽어 CM 코어 내의 전역 구조체 버퍼(`stIpcRxFromCpu1`)에 즉각 저장.

### Phase 5: CM 코어 - 이더넷 W6100 수신 인터럽트 및 데이터 응답
- **대상**: `TMDSCNCD28388D_T_CM` (`hal_Ethernet.c`, `csu_Ethernet.c` 등)
- **내용**:
  - 현재 `main.c`의 `while(1)`에서 폴링 기반으로 패킷을 수신(recv)하는 로직 제거.
  - W6100 소켓의 수신(Rx) 하드웨어 인터럽트 활성화 및 ISR 구성.
  - Rx ISR 내에서 PC의 "데이터 요청" 패킷을 파싱.
  - 수신된 요청이 유효하면, Phase 4에서 저장해 둔 `stIpcRxFromCpu1` 전역 구조체 버퍼 데이터를 PC로 송신(Send).

### Phase 6: PC 프로그램 - 100ms 요청 송신 및 실시간 그래프 UI 추가
- **대상**: C# 프로젝트 (UI 폼, `SciPcProtocol.cs` 등 통신 모듈)
- **내용**:
  - 화면에 "데이터 모니터링 (100ms)" 버튼 및 C# `Timer` 컴포넌트 추가.
  - 타이머 활성화 시 100ms 주기로 패킷(데이터 요청 메세지) 타겟 보드로 발송.
  - 패킷 수신 이벤트 핸들러에서 전달받은 바이트 배열을 역직렬화하여 구조체 형태로 복원.
  - 폼에 `System.Windows.Forms.DataVisualization.Charting` (Chart Control)을 추가하고, 수신된 사인파 값과 ADC 값을 실시간 그래프에 Plot.

---

## 3. 유의 및 검증 사항 (Verification Plan)
- **플랫폼 분리 원칙**: C2000(CPU1)과 ARM(CM)의 레지스터 구조 및 제어 방식이 완전히 다르므로, 각각의 컴파일러 규격 및 데이터 타입 크기(16bit vs 8bit align)를 철저하게 교차 검증하며 코드를 분리 적용합니다.
- **Data Alignment (CWE-120 등 방어)**: IPC 통신 시 공유 메모리의 구조체 패딩(Padding) 및 정렬 불일치로 인한 오작동을 방지하기 위해 패킹(`#pragma PACK` 또는 `__attribute__((packed))`) 여부와 메모리 오프셋을 점검합니다.
- **정적 시험(품질 메트릭) 준수**: ISR 내부에서 과도한 루프나 복잡도를 가지지 않도록 기능을 모듈화하고 순환 복잡도(20 이하)를 유지합니다.

---

> [!NOTE]
> 본 Plan 문서에 이견이 없으시다면 **"승인"** 또는 **"구현 시작해"**라고 말씀해 주십시오. Phase 1부터 차례로 구현을 진행하겠습니다.
