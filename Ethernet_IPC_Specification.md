# 🌐 TMDSCNCD28388D_T 통신 및 인터페이스 명세서 (Ethernet & IPC)

본 문서는 TMS320F28388D 듀얼 코어 시스템에서 사용되는 **내부 코어 간(IPC/MSGRAM) 통신 구조** 및 **외부 기기(PC)와의 이더넷(UDP) 프로토콜**에 대한 모든 설정 및 데이터 규격을 통합 정리한 스펙 문서입니다.

---

## 1. MSGRAM (Message RAM) 물리 주소 맵핑
CM 코어의 GSRAM 쓰기 제약(Hard Fault)을 극복하기 위해, 양방향 전용으로 분리된 MSGRAM을 통해 Lock-Free 동기화 통신을 수행합니다.

| 방향 | 용도 | CPU1 관점 물리 주소 | CM 관점 물리 주소 | 크기 |
| :--- | :--- | :--- | :--- | :--- |
| **CPU1 ➡️ CM** | CPU1의 모터 제어/센서 데이터를 이더넷 송신용으로 CM에 전달 | `0x39000` (`CPU1TOCMMSGRAM`) | `0x20080000` | 2 KB |
| **CM ➡️ CPU1** | 이더넷 수신(PC의 제어 명령) 데이터를 CPU1에 전달 | `0x38000` (`CMTOCPU1MSGRAM`) | `0x20082000` | 2 KB |

---

## 2. 공유 데이터 구조체 (IPC & MSGRAM Payload)
`csu_Ipc_cpu1.h` 및 `csu_Ipc_cm.h` 내에 정의된 `stIpcDataPacket` 구조체 기준입니다.

### 2.1. 전체 데이터 패킷 구조
```c
typedef struct {
    uint32_t seqCount;      // [4B] 동기화용 Seqlock 카운터 (짝수=완료, 홀수=쓰기중)
    uint32_t Command;       // [4B] 명령어
    uint32_t Status;        // [4B] 상태 플래그
    uint32_t Address;       // [4B] 보조 메모리 주소
    union {
        uint32_t PayloadRaw[16];   
        TxData_t TxData;    // CPU1 -> CM 구조체
        RxData_t RxData;    // CM -> CPU1 구조체
    } Payload;
} stIpcDataPacket;
```

### 2.2. Payload 세부 구조
**A. `TxData` (CPU1 ➡️ CM)**
- `waveValue` (`float32_t` / 4B) : 메인 루프 연산 파형 데이터 (Sine, Square, Triangle)
- `adcTemperature` (`float32_t` / 4B) : ADC 측정 내부 온도
- `sequenceNum` (`uint32_t` / 4B) : 데이터 갱신 시퀀스 번호

**B. `RxData` (CM ➡️ CPU1)**
- `seqNum` (`uint32_t` / 4B) : PC로부터 수신받은 제어 시퀀스 (이더넷 패킷에서 1B 수신 후 대입)
- `waveType` (`uint32_t` / 4B) : PC로부터 수신받은 파형 종류 (0: Sine, 1: Square, 2: Triangle) (이더넷 패킷에서 1B 수신 후 대입)

### 2.3. 동기화 메커니즘 (Lock-Free)
- **CPU1 쓰기 (Seqlock)** : 데이터 기록 전 `seqCount`를 홀수로 만들고, 쓰기 완료 후 짝수로 복귀.
- **CM 읽기 (Spin-Lock)** : `seqCount`가 홀수면 대기(Spin)하고, 짝수일 때 읽습니다. 단, 읽는 도중 값이 바뀌면 다시 읽어 데이터 찢김(Tearing)을 원천 차단합니다.
- **CPU1 읽기 (Try-Lock)** : 100us 실시간성(Jitter 0) 보장을 위해 `seqCount`가 홀수면 대기하지 않고 즉시 Pass하여, 이전 주기의 데이터를 그대로 활용합니다.

---

## 3. 이더넷 (UDP) 네트워크 설정
| 항목 | DSP (TMS320F28388D) | PC (제어/모니터링 장치) |
| :--- | :--- | :--- |
| **IP 주소** | `192.168.200.10` | `192.168.200.100` |
| **MAC 주소** | `A8:63:F2:00:38:88` | Auto-Learning (수신 패킷 기반 동적 캡처 갱신) |
| **수신(RX) 포트** | `5001` 고정 대기 | `5001` 또는 Auto-Learning |
| **프로토콜** | IPv4, UDP (Little Endian Payload) | IPv4, UDP |

---

## 4. 이더넷 프로토콜 (UDP Payload 규격)
모든 Payload 데이터 필드는 **Little Endian**을 사용합니다.
**CheckSum 규칙**: 자신(Checksum 필드)을 제외한 앞선 모든 페이로드(Header + Data) 바이트 값들을 단순 합산한 결과의 **최하위 2바이트 (Little Endian)**.

### 4.1. 공통 MSG Header (12 Bytes)
PC $\leftrightarrow$ DSP 간 모든 UDP 패킷의 최상단 12바이트는 공통 헤더 규격을 사용합니다.
- `[0~3]` **Timestamp** (4B) : 패킷 송수신 시간 동기화 식별자
- `[4]` **Source ID** (1B) : `0x10` (DSP) / `0x20` (PC)
- `[5]` **Dest ID** (1B) : `0x10` (DSP) / `0x20` (PC)
- `[6]` **Code** (1B) : `0x10` (Monitor 요청/응답), `0xFF` (ACK 응답) 등
- `[7]` **Request ACK** (1B) : `0x00` (요청), `0x01` (NACK), `0xFF` (응답 / ACK 불필요)
- `[8]` **Priority** (1B) : `0x00` (Normal)
- `[9]` **Send Count** (1B) : `0x00` (기본값)
- `[10~11]` **Data Length** (2B) : 뒤이어 오는 순수 Data 길이 (Header/Checksum 제외, 바이트 단위)

### 4.2. DSP ➡️ PC (Reflect 패킷 - 모니터링 응답)
PC의 데이터 요청(Code `0x10`)에 대해 DSP가 실시간 상태 데이터를 송신하는 응답 패킷입니다.
- **총 Payload 크기**: 22 Bytes (Header 12 + Data 8 + Checksum 2)
- **Data 필드 상세 (8B)**:
  - `[12]` **Sequence Num** (1B)
  - `[13]` **WaveType** (1B, Default 0)
  - `[14~15]` **DspTemp** (2B) : `float32` 온도를 10배 곱한 후 정수 캐스팅한 값 (`uint16`)
  - `[16~19]` **WaveVal** (4B) : IEEE-754 표준 Float32 바이너리 덤프 데이터 (선택된 파형 값)

### 4.3. PC ➡️ DSP (Update 패킷 - 제어 요청)
PC가 DSP의 상태를 변경하거나 모니터링 전송을 요청할 때 전송하는 패킷입니다.
- **총 Payload 크기**: 16 Bytes (Header 12 + Data 2 + Checksum 2)
- **Data 필드 상세 (2B)**:
  - `[12]` **Sequence Num** (1B)
  - `[13]` **WaveType** (1B) : 요청할 파형 (0: Sine, 1: Square, 2: Triangle)

### 4.4. DSP ➡️ PC (ACK / NACK 패킷)
PC로부터 Update 패킷 수신 시 즉시 성공/실패(Checksum 등) 여부를 응답하는 패킷입니다.
- **총 Payload 크기**: 18 Bytes (Header 12 + Data 4 + Checksum 2)
- **Data 필드 상세 (4B)**:
  - `[12]` **Target Code** (1B) : 수신한 원본 메시지의 Code 값
  - `[13]` **Reserved** (1B) : `0x00`
  - `[14~15]` **Ack Info** (2B) : `0x0000` (성공/OK), `0x0001` (Checksum Error)

---

> **💡 개발자 비고**
> 본 명세서는 F28388D 듀얼 코어의 **내부 통신(MSGRAM) 아키텍처**와 **외부 통신(Ethernet UDP)** 사양이 결합된 통합 레퍼런스 가이드입니다.
> - C# 등 외부 언어로 PC 프로그램을 개발할 때, 이 문서의 3장과 4장을 기준으로 패킷 파서(Parser) 및 빌더(Builder)를 작성하면 됩니다.
> - DSP 코어 간의 새로운 데이터 공유가 필요할 경우, 2.2장의 `TxData` 또는 `RxData` 구조체에 변수를 추가하고, `csu_Ipc_cpu1.h` 및 `csu_Ipc_cm.h`를 갱신하십시오.
