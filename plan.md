# 📋 정적 분석 및 리팩토링 계획서 (Static Analysis & Refactoring Plan)

**작성 일자**: 2026. 06. 05.  
**현재 단계**: Phase 2 - 소스코드 정적 분석 및 취약점 방어 코딩 적용  
**대상 코어**: CPU1 Core 및 CM Core (`main.c`, `CSU`, `Dev` 폴더 전체)  

---

## 1. 개요 및 목표 (Goal Description)
사용자 정의 규칙에 따라 현재 구현된 모든 소스 코드를 대상으로 **DAPA SCR-G (무기체계 코딩규칙), 소스코드 품질 메트릭, CWE 보안 취약점 표준**을 전수 검사하고, 규격에 미달하는 코드를 리팩토링합니다. 기존의 동작 로직은 **절대 변경하지 않으며**, 오직 코드의 구조적 안정성과 방어력만 끌어올리는 것이 목표입니다.

---

## 2. 점검 및 조치 기준 (Verification Metrics & Targets)

### A. 무기체계 소프트웨어 코딩규칙 (DAPA SCR-G)
- **단일 종료점 (Single Exit Point)**: 함수 내의 `return` 문은 오직 맨 마지막에 1개만 허용됩니다. (중간 return 불가)
- **예외 방어 블록 필수화**: 모든 `switch` 문에는 `default:` 블록을, 모든 `if ... else if` 문에는 `else` 블록을 명시적으로 작성하여 예기치 않은 상태를 방어합니다.
- **초기화 강제**: 모든 지역 변수는 선언과 동시에 명시적 초기값(0, NULL, false 등)을 할당받아야 합니다.

### B. 소스코드 품질 메트릭 (복잡도/신뢰성 제한)
- **순환 복잡도 (Cyclomatic Complexity)**: `<= 20` (분기문 갯수 제한)
- **함수 호출 최대 깊이 (Call Levels)**: `<= 6` (중첩 호출 제한)
- **함수 매개변수 수 (Parameters)**: `<= 8`
- **호출/피호출 수 (Fan-in / Fan-out)**: Fan-in `<= 8`, Fan-out `<= 10`
- **실행 가능 코드 라인 수**: `<= 200` 라인 (선언부 제외)

### C. CWE-658 / 659 보안 취약점 점검
- **CWE-120 (Buffer Overflow)**: 배열 인덱스/포인터 연산 전 범위 초과 여부 검사.
- **CWE-476 (Null Pointer Deref.)**: 포인터 사용 전 반드시 `NULL` 체크 적용.
- **CWE-369 (Divide by Zero)**: 나눗셈 연산 전 분모가 `0`인지 확인하는 방어 분기 추가.
- **CWE-457 (Uninitialized Var)**: 변수 선언 시 쓰레기값 방어용 초기화.
- **CWE-190 (Integer Overflow)**: 변수 자료형 범위를 초과하는 누적/곱셈 연산 방어.
- **상수 타입 명시화 (MISRA / DAPA)**: Unsigned 변수 연산이나 할당에 사용되는 모든 상수 리터럴에는 반드시 `u` 또는 `U` 접미사를 강제하여 암시적 형변환(Implicit Conversion) 오류를 원천 차단합니다. (예: `1000` ➡️ `1000u`)

---

## 3. 리팩토링 수행 계획 (Proposed Changes)

검토 후 규격을 위반한 함수가 발견되면 다음과 같은 순서로 리팩토링을 수행합니다.

### 1단계: CPU1 코어 (Control Core) 전수 검증 및 리팩토링
#### [MODIFY] `main.c` (CPU1)
- 메인 루프(`for(;;)`) 및 타이머 백그라운드 태스크 내부의 제어 흐름 복잡도 점검.
- 누락된 `else` 블록에 방어 주석 추가.

#### [MODIFY] `CSU/*` 및 `Dev/*` (CPU1)
- 예: `CSU_SCI_PC.c` 등에서 중간 `return`이 발견되면 결과값을 저장하는 지역 변수(`ret` 또는 `result`)를 도입하고 마지막에 `return ret;` 하도록 구조 변경.
- 나눗셈/모듈러 연산(`%`) 사용 시 0 나누기 방어 코드 삽입.

---

### 2단계: CM 코어 (Connectivity Core) 전수 검증 및 리팩토링
#### [MODIFY] `main.c` (CM)
- 이더넷 폴링 루프 등 통신 처리 루프의 복잡도 및 Array Bounds 체크 확인.

#### [MODIFY] `CSU/*` 및 `Dev/*` (CM)
- CM용 Driverlib API 및 8비트 주소 체계 호환성 확인.
- `CSU_Ethernet.c` 내의 포인터 파라미터(`uint8_t *` 등) 역참조 전 `if (ptr != NULL)` 방어 코드 강제 적용.
- 멀티플 `return`을 단일 `return`으로 통일.

---

## 4. 사용자 피드백 요청 (User Review Required)

> [!IMPORTANT]
> - 코어 로직 자체는 변경되지 않지만, 단일 `return` 원칙과 방어적 `else`/`default` 추가로 인해 코드 라인수가 다소 증가하고 변수 선언 구조가 바뀔 수 있습니다. 
> - 이 계획에 승인(Approval)해 주시면, 바로 CPU1 코어의 파일들부터 스캐닝 및 코드 갱신 작업을 시작하고 진행률을 `task.md`를 통해 추적하겠습니다.
> - 계획서(`plan.md`)에 추가하고 싶으신 메모나 제약 조건이 있다면 자유롭게 적어주세요.

---

## 5. 최종 검증 계획 (Verification Plan)

### 정적 검증 (자체 검사)
- 리팩토링 후 각 함수의 순환 복잡도를 역산하여 20 이하인지 확인.
- 모든 파일에서 중간 `return`이 남아있지 않은지 `grep` 스캔 확인.

### 컴파일 및 동적 검증 (사용자 수행)
- CPU1 및 CM 코어 전체 빌드(Build) 수행 시 문법 에러 유무 확인.
- 통신(Ethernet, IPC, SCI) 및 센서 획득(ADC) 동작이 기존과 동일하게 무결하게 동작하는지 테스트.
