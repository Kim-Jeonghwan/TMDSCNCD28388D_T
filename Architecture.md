# TMDSCNCD28388D_T 시스템 전체 명세서 (Project Specification)

본 문서는 TMS320F28388D 듀얼 코어(CPU1 + CM) 기반 펌웨어 및 PC 프로그램의 전체 아키텍처와 세부 기능 스펙을 정리한 문서입니다. 
새로운 기능이 구현될 때마다 본 문서를 지속적으로 업데이트하여 시스템의 현재 상태를 완벽하게 추적합니다.

*   **Last Updated**: 2026. 06. 22. (MSGRAM 기반 통신 전환 및 Lock-Free 동기화 반영)

---

## 1. 하드웨어 및 시스템 기본 설정
- **타겟 보드** : TMS320F28388D (ControlCARD)
- **보드 주파수 (CPU1 / CPU2)** : **200 MHz** (`25MHz(XTAL) * 32 / 4`)
- **CM 코어 설정 주파수** : **125 MHz** (`AUXPLL` 소스 기반, `25MHz * 40 / 8 = 125MHz`)
- **발진기(크리스탈) 주파수** : **25 MHz** (외부 XTAL)
- **부팅 및 동기화 시퀀스** :
  - **CPU1**: `DSP_Initialization()` 이후 `Initial_IPC()`로 하드웨어 동기화 수행. 전역 인터럽트 활성화 후 CM 코어의 `READY` 통보를 무한 대기하며 록업을 방지함.
  - **CM**: `CM_init()` 및 인터럽트 램 복사 완료 후 `Initial_IPC()` 핸드셰이크 수행. 이더넷 PHY 안정화를 위해 100ms 하드웨어 딜레이 대기 후 주변장치 기동. 최종적으로 CPU1에 `READY` 통보.

---

## 2. 코어 간 통신 (IPC 및 공유 메모리 MSGRAM) 스펙
- **통신 아키텍처**: 부팅 동기화 등 1회성 상태 제어에는 하드웨어 메일박스(IPC)를 사용하며, 실시간성이 요구되는 모터 제어 및 이더넷 송수신 데이터 교환에는 **공유 메모리 맵핑 (MSGRAM 폴링 방식)**을 활용한 Lock-Free 공유 메모리 폴링 방식을 채택했습니다.

- **공유 메모리 맵핑 (MSGRAM 폴링 방식)**:
  - `IPC_CPU1_TO_CM_MSGRAM_ADDR`: CPU1(Writer) → CM(Reader).
  - `IPC_CM_TO_CPU1_MSGRAM_ADDR`: CM(Writer) → CPU1(Reader).
  - *참고*: 초기에 GSRAM(GS0/GS1)을 사용하려 했으나, F2838x 하드웨어 설계상 **CM 코어는 GSRAM에 대한 쓰기 권한이 없으므로(Hard Fault 유발)**, 양방향 쓰기가 가능한 전용 **MSGRAM(Message RAM)**으로 롤백하여 Lock-Free 동기화를 적용하였습니다. MSGRAM 역시 메모리 맵 방식으로 접근 가능하므로 IPC 인터럽트를 유발하지 않고 폴링/Seqlock 적용이 가능합니다.

### 2.1. 실시간 데이터 공유 (MSGRAM 기반 Lock-Free)
- **메모리 분리**: MSGRAM은 하드웨어적으로 방향이 고정되어 있으므로 (CPU1->CM, CM->CPU1) 별도의 마스터십 설정이 불필요하며 메모리 충돌이 원천 방지됩니다.
- **CPU1 ➡️ CM 전송 (CPU1TOCM MSGRAM)**
  - **주기**: **100us** (CPU1의 EPWM1 메인 제어 루프)
  - **데이터 구조체**: `pxDataCpu1ToCm->Payload.TxData` 사용 (사인파 `float32_t`, 온도 `float32_t`, 시퀀스 `uint32_t`)
  - **동기화 (Seqlock)**: 100us마다 ADC 및 사인파 연산 직후, CPU1이 `seqCount` 카운터를 홀수(쓰기 중)로 만들고 데이터를 쓴 뒤 짝수(쓰기 완료)로 돌려놓습니다. CM은 짝수일 때만 읽으며, 만약 CM이 읽는 도중 값이 변경되면 다시 읽어 데이터 찢김(Tearing)을 완벽히 방지합니다.
- **CM ➡️ CPU1 전송 (CMTOCPU1 MSGRAM)**
  - **주기**: 이더넷 패킷 수신 시 즉각 반영 (최대 10ms~100ms 가변)
  - **데이터 구조체**: `pxDataCmToCpu1->Payload.RxData` 사용 (`seqNum`, `status`)
  - **동기화 (Try-Lock)**: CM이 MSGRAM에 기록할 때 Seqlock을 사용하며, CPU1은 100us 루프에서 읽기를 시도할 때 CM이 데이터 기록 중(홀수)이라면 **기다리지 않고 바로 포기(Pass)**하여 메인 제어 루프의 지연(Jitter)을 0으로 보장합니다.

### 2.2. 제어 명령 통신 (IPC 메시지 램)
- **방식**: CM 코어 기동 완료 통보(`IPC_CMD_CM_BOOT_READY`) 등 시스템 제어 명령에 한해서만 메일박스 인터럽트 플래그를 통한 비동기 전송을 유지합니다.

---

## 3. 이더넷 (Ethernet / UDP) 스펙
- **PHY IC** : DP83822IRHBR
- **PHY 연결 모드** : **MII 모드** (`ETHERNET_SS_PHY_INTF_SEL_MII`)
- **MII 연결 핀 (MUX)** : 
  - **TX**: GPIO44(CLK), 118(EN), 75(D0), 122(D1), 123(D2), 124(D3)
  - **RX**: GPIO111(CLK), 112(DV), 113(ERR), 114(D0), 115(D1), 116(D2), 117(D3)
  - **MDIO/기타**: GPIO105(MDC), 106(MDIO), 108(Reset), 109(CRS), 110(COL)
- **PHY LED 제어 (DP83822 하드웨어 특성 반영)** :
  - **LED_1 (초록색)**: 부트스트랩 저항으로 인해 Tri-state 모드로 고정되는 현상을 방지하기 위해, 확장 레지스터 **`0x0462 (IOCTRL1)`**의 MUX 비트를 제어하여 강제로 LED 모드로 개방함.
  - **LED_0 & LED_1 동작 모드**: 확장 레지스터 **`0x0460 (LEDCFG1)`**을 사용하여 두 LED 모두 `Link OK 및 TX/RX Activity 깜빡임` 모드로 활성화함.
  - **LED_1 강제 점등 (오버라이드)**: **`0x0469 (LEDCFG2)`** 레지스터를 사용하여 LED_1 논리 상태를 1(High)로 강제 오버라이드. 3.3V 풀업에 의한 Active Low(0) 극성이 적용되어 실제 핀은 LOW로 출력되며, 이에 따라 초록색 LED가 강제로 점등됨.
- **IP 설정** : 고정 IP (DSP: `192.168.200.10`, PC: `192.168.200.100`)
- **MAC 어드레스** :
  - DSP: `A8-63-F2-00-38-88`
  - PC: 초기값 `EC-9A-0C-14-E8-4B` ➡️ **런타임 시 5001번 포트로 들어오는 패킷을 분석해 동적 캡처(Auto-learning)하여 갱신**
- **UDP 포트 및 필터링** : `5001`번 포트로 수신되는 패킷만 통과시켜 OS 백그라운드 잡음(1947번 포트 등) 방어.
- **프로토콜 페이로드 (22 Bytes)** :
  - Header(12B) + Data(8B: Seq, Status, DspTemp, SineVal) + Checksum(2B)
- **구동 아키텍처** :
  - **RX (수신)**: CM 코어 EMAC 하드웨어 인터럽트(`INT_EMAC_RX0`) 기반 처리로 개편하여 폴링 부하를 제거했습니다. 수신된 패킷에서 추출한 제어값(`SeqNum`, `Status`)은 CPU1로 즉각 전달하기 위해 **CMTOCPU1 MSGRAM**에 Seqlock 방식으로 안전하게 기록합니다.
  - **TX (송신)**: PC로부터 데이터 요청(Monitoring) 패킷이 수신되었을 때만 응답 패킷(Reflect MSG)을 조립하여 송신하는 구조입니다. CPU1이 지속적으로 갱신 중인 최신 상태값(온도, 사인파)을 **CPU1TOCM MSGRAM**에서 Seqlock Spin-Lock으로 찢김(Tearing) 없이 가져옵니다.
  - **큐잉(Queueing) 방어**: 4개의 TX 및 RX 디스크립터 풀을 순환 참조(Circular)하여 Linked-List 꼬임 및 메모리 릭 원천 방지.
- **PC 모니터링 시스템 스펙** :
  - **비동기 요청**: C# WinForms 기반의 100ms 자동 요청 타이머 구동을 통해 주기적으로 데이터를 폴링합니다.
  - **실시간 렌더링**: `ScottPlot` 라이브러리를 활용하여, 수신된 패킷에서 역직렬화된 사인파(Float) 및 온도 데이터를 초고속 2채널 차트로 렌더링합니다.

---

## 4. 아날로그 센서 (ADC) 스펙
- **측정 대상** : DSP 내부 정션 온도 센서 (채널: **ADCIN13**)
- **ADC 모듈** : **ADCA** (SOC2 사용)
- **물리적 샘플링(트리거)** : **ePWM9 SOCA 하드웨어 트리거** (온도 센서 전담을 위해 별도 분리, 느리고 안정적인 **1kHz(1ms)** 주기로 동작)
- **데이터 획득** : ADCA 인터럽트(`AdcaIsr`)에서 즉시 원시 데이터(`adcResult`) 획득
- **연산 처리 주기** : 기존의 10ms 폴링 방식에서 **CPU1 EPWM1 하드웨어 인터럽트(100us)** 내부에서 즉각 연산 및 처리하는 방식으로 고도화되어 실시간 응답성이 극대화되었습니다.
- **변환 공식** : C2000Ware 내장 `scaleFactor`, `tempOffset`, `tempSlope` 적용 (Divide by Zero 방어 적용).
- **필터링 (노이즈 방어)** :
  - ADC 고주파 전기적 노이즈로 인한 소수점 요동을 막기 위해 **IIR Low-Pass 필터 (Alpha = 0.05)** 탑재.

---

## 5. 시스템 상태 LED (System UI) 스펙
- **RUN LED (GPIO 31)** : 시스템 정상 동작(Run)을 나타내며 1초 주기로 토글되어 깜빡입니다.
- **인터럽트 동기화 검증 LED (GPIO 34)** : CPU1 메인 제어 루프인 `EPWM1 100us 인터럽트(ISR)` 내에서 실시간으로 토글(10kHz 주기)됩니다. 오실로스코프를 이용해 물리적인 인터럽트 주기와 동작 상태를 검증하기 위한 디버깅 핀으로 사용됩니다.
- **제어 아키텍처 (ATTLA_T 동기화)** :
  - **계층 분리**: LED 핀의 방향 설정 및 초기화(Mux)는 철저히 하드웨어 추상화 계층인 HAL(`hal_DspInit.c`)에서 수행하며, 논리적인 점멸 제어는 CSU(`csu_LED.c`) 계층에서 수행하여 하드웨어 의존성을 분리했습니다.
  - **안정성 확보**: 기존 C2000 구조에서 16비트 메모리 경계 오버플로우를 유발할 수 있는 비트필드 구조체(`:8u`, `:1u`)를 제거하고, 자료형을 `uint16_t`로 통일했습니다. 불필요한 미사용 LED(GPIO 145, 146 등)를 제거해 제어 루프를 최적화했습니다.

---

## 6. 백그라운드 태스크 스케줄러 (Timer) 스펙
시스템의 유휴 시간에 다양한 비동기 작업을 처리하기 위해, 하드웨어 타이머와 소프트웨어 스케줄러를 결합하여 구동합니다.
- **하드웨어 타이머 계층 (`hal_Timer.c`)** :
  - `CPUTimer0` : **100 us** 인터럽트 (초고속 스케줄링 예약용)
  - `CPUTimer1` : **1 ms** 인터럽트 (라운드 로빈 스케줄러용 시스템 틱 증가)
  - `CPUTimer2` : **1000 ms** 인터럽트 (CPU 코어별 동작 주파수 동기화 및 Hz 계수 측정)
- **소프트웨어 스케줄러 (`main_cpu1.c`, `main_cm.c`)** :
  - **1ms Task**: (CPU1) 동작 주파수(Hz) 측정, (CM) 이더넷 통신 활성화 상태 소등 타이머(RJ-45 흉내).
  - **10ms Task**: (CPU1) PC로 온도 및 시퀀스 상태를 전송하는 SCI 패킷 비동기 송신.
  - **100ms Task**: (CPU1) 보드 시스템 LED 점멸 상태 갱신, (CM) GPIO 145번 LED 점멸 제어.
  - **1000ms Task**: 저속 에러 진단 및 워치독 클리어(예약).

---

## 7. 직렬 통신 (SCI / UART PC 모니터링) 스펙
레거시 PC 모니터링 및 상태 디버깅을 지원하기 위한 직렬 프로토콜 사양입니다.
- **하드웨어 계층 (`hal_Sci.c`)** : 
  - **모듈**: SCIA (Baudrate 115200, 8-N-1 설정)
  - **핀 맵핑**: GPIO28(RXD, Pull-Up 적용), GPIO29(TXD)
- **통신 프로토콜 (`csu_SciPc.c`)** :
  - **프레임 구조**: `SOF(0x7E)` ➡️ `MSGID(0x10)` ➡️ `LEN` ➡️ `DATA(4 Bytes)` ➡️ `Checksum` ➡️ `EOT(0x0D)` 형태의 완전한 폐쇄형 구조.
  - **페이로드(DATA)**: 시퀀스 카운터, 디바이스 상태(Status), 그리고 x10 스케일링이 적용된 온도(`DspTemp`) 2바이트로 구성됨.
- **송수신 아키텍처** :
  - **수신 (RX)**: SCIA 하드웨어 인터럽트 내에서 1바이트씩 FIFO를 읽어 Frame State Machine을 거치며 안전하게 패킷을 조립.
  - **송신 (TX)**: 10ms 라운드 로빈 스케줄러를 통해 데이터를 원형 버퍼(Circular Queue)에 적재(Enqueue)하고, 유휴 메인 루프에서 비동기로 물리적 송신(Dequeue)을 수행하여 통신 지연으로 인한 메인 제어 루프 블로킹 방어.

---

## 8. 기타 통신 및 제어 모듈 스펙 (SPI / EPWM)
- **SSI 엔코더 통신 (SPI) (`hal_Spi.c`)** :
  - **모듈**: SPIC (1MHz, 16비트 워드, Mode-2: POL1/PHA0)
  - **핀 맵핑**: GPIO 51(SOMI), GPIO 52(CLK) (Master 모드로 외부 SSI 엔코더에서 위치 데이터 획득용 예약)
- **보조 EPWM 제어 (`CSU_EPWM.c`)** :
  - **모듈**: EPWM7A (GPIO 12)
  - **용도 및 초기화**: 보조 펄스 출력을 위해 Up-count 모드로 설정. 초기 부팅 시에는 Force Low를 강제하여 의도치 않은 출력을 차단.

---

## 9. 소프트웨어 아키텍처 (CSU / HAL 계층 구조)
본 프로젝트는 펌웨어 유지보수성과 하드웨어 종속성 탈피를 위해 엄격한 **CSU (Control & Service Unit)** 및 **HAL (Hardware Abstraction Layer)** 3계층 구조를 따릅니다. 

### 9.1. CPU1 코어 파일 아키텍처
- **Main (`main_cpu1.c`, `main_cpu1.h`)**: 시스템 진입점으로 하드웨어 초기화(`DSP_Initialization`) 및 전역 인터럽트 제어를 담당하며, 백그라운드 태스크(1ms~1s) 스케줄링을 관장.
- **CSU (어플리케이션 계층)**: 순수 비즈니스 로직. 레지스터 접근 절대 금지.
  - `csu_Control.c`: 시스템의 심장인 100us 메인 제어 루프. ADC 갱신, 사인파 연산 및 MSGRAM을 통한 데이터 교환(Seqlock / Try-Lock 동기화) 총괄.
  - `csu_Adc.c`: 센서 데이터의 물리적 의미 변환(스케일링) 및 필터링.
  - `csu_LED.c`: LED 점멸 알고리즘 및 상태 머신.
  - `csu_SciPc.c`: PC 직렬 통신 프로토콜 패킷 페이로드 조립 및 파싱.
  - `csu_Ipc_cpu1.c`: MSGRAM 포인터 매핑(`pxDataCpu1ToCm`, `pxDataCmToCpu1`) 및 통신 데이터 구조체 정의.
  - `CSU_EPWM.c`: EPWM 제어 로직, Duty 및 주파수 알고리즘 처리.
- **HAL (하드웨어 추상화 계층)**: 레지스터, Driverlib 캡슐화. 인터럽트(ISR)와 하드웨어 핀 제어 수행.
  - `hal_DspInit.c`: GPIO Mux, 클럭 설정, CM 코어 부팅 트리거링.
  - `hal_Epwm.c`, `hal_Adc.c`: 타이머, 클럭 분주비, 트리거 소스, ADC 하드웨어 셋업.
  - `hal_Sci.c`: SCI 레지스터 설정, RX FIFO 인터럽트 등록 및 비동기 원형 큐 관리.
  - `hal_Spi.c`: SPI(SSI 엔코더) 통신 핀 및 레지스터 속도 설정.
  - `hal_Timer.c`: CPU 타이머(0, 1, 2) 인터럽트(ISR) 및 클럭 분주 기본 초기화.
  - `hal_Ipc_cpu1.c`: IPC 플래그 록업 제어 및 메일박스 인터럽트 플래그 설정.
  - `hal_Ramfuncs.c`: 플래시 메모리 병목 현상 방지를 위한 성능 최적화용 함수 RAM 적재 지원.
  - `hal_Common.c`: 공통 자료형 선언 및 시스템 공통 유틸리티 모음.

### 9.2. CM 코어 파일 아키텍처
- **Main (`main_cm.c`, `main_cm.h`)**: CM 코어 부팅 진입점. 이더넷 딜레이 확보 및 CPU1에 Boot Ready 상태 통보, 라운드 로빈 태스크 제어.
- **CSU (어플리케이션 계층)**: 
  - `csu_Ethernet.c`: UDP 데이터 소켓 프로토콜, MAC 주소 자동 캡처, 그리고 PC 요청에 대한 응답용 Reflect MSG 패킷 조립 및 MSGRAM 메모리 접근 전담.
  - `csu_Ipc_cm.c`: MSGRAM 포인터 매핑 및 이더넷 송수신 버퍼 데이터 구조 선언.
- **HAL (하드웨어 추상화 계층)**: 
  - `hal_Ethernet.c`: ARM Cortex-M4 전용 EMAC 디바이스 드라이버 로직, RX/TX 디스크립터 풀 관리, MDIO 통신.
  - `hal_Ipc_cm.c`: 메시지 램(Message RAM) 인터럽트 셋업 및 IPC 플래그 상태 제어.
  - `hal_Timer.c`: CM 코어 전용 타이머 주기 설정 및 라운드 로빈용 틱 카운터(ISR) 등록.

---

## 10. 향후 추가 구현 예정 기능
*여기에 모터 제어 등 새로운 기능이 구현될 때마다 내용이 누적됩니다.*
- **완벽한 모터 제어 알고리즘 구현**: 현재 테스트용 사인파(Sine wave)를 생성하여 통신하는 구조에서 나아가, 실제 인버터 PWM 스위칭 및 FOC(Field Oriented Control) 등 정밀 모터 제어 루틴을 `csu_Control.c`의 100us 주기에 본격적으로 탑재할 예정입니다.
