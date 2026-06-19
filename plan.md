# 파일명 및 모듈명 리팩토링 구현 계획 (Implementation Plan)

## 1. 개요
프로젝트(CPU1, CM) 내부의 `csu_`, `hal_` 모듈들에 대한 명칭을 규칙에 맞게 변경하고, 관련된 헤더 선언, include, 헤더 가드 등을 빠짐없이 갱신하기 위한 구현 계획입니다.

## 2. 작업 절차 (Action Plan)

### Step 1: 파일 내용(주석, #include, 헤더 가드) 전면 교체
파일 이름 자체를 변경하기 전에, 파일 **내부의 텍스트**를 새로운 이름에 맞추어 안전한 도구를 사용하여 국소적으로 치환(Replace)합니다. 
*임베디드 펌웨어 수정 규칙에 의거, 해당 내용은 일괄 수정되며 한글 주석이 깨지지 않도록 UTF-8 인코딩에 유의합니다.*

- **CPU1 헤더 가드 및 주석 변경**
  - `CSU/csu_EPWM.h`, `CSU/csu_EPWM.c`
  - `CSU/csu_IPC.h`, `CSU/csu_IPC.c`
  - `CSU/csu_LED.h`, `CSU/csu_LED.c`
  - `CSU/csu_SCI_PC.h`, `CSU/csu_SCI_PC.c`
  - `HAL/hal_EpwmTimer.h`, `HAL/hal_EpwmTimer.c`
  - `HAL/hal_IPC.h`, `HAL/hal_IPC.c`
- **CM 헤더 가드 및 주석 변경**
  - `CSU/csu_IPC.h`, `CSU/csu_IPC.c`
  - `HAL/hal_IPC.h`, `HAL/hal_IPC.c`
- **타 모듈 참조 주석 변경**
  - `CPU1/HAL/hal_DspInit.c`
  - `CM/CSU/csu_Ethernet.h`, `CM/CSU/csu_Ethernet.c`

### Step 2: main.h 내의 #include 구문 일괄 교체
- `CPU1`의 `main.h`: 구 파일명들을 새 파일명들로 치환.
- `CM`의 `main.h`: 구 파일명들을 새 파일명들로 치환.

### Step 3: 파일 이름 변경 (Powershell `Rename-Item`)
내부 텍스트 치환이 완료되면 터미널 명령어를 통해 실제 파일의 이름을 변경합니다. (Git 자동 연동 금지 룰에 따라 Powershell 기본 명령어로 변경합니다.)
- **CPU1 Rename-Item 목록**
  - `csu_EPWM.*` ➔ `csu_Epwm.*`
  - `csu_IPC.*` ➔ `csu_Ipc_cpu1.*`
  - `csu_LED.*` ➔ `csu_Led.*`
  - `csu_SCI_PC.*` ➔ `csu_SciPc.*`
  - `hal_EpwmTimer.*` ➔ `hal_Epwm.*`
  - `hal_IPC.*` ➔ `hal_Ipc_cpu1.*`
- **CM Rename-Item 목록**
  - `csu_IPC.*` ➔ `csu_Ipc_cm.*`
  - `hal_IPC.*` ➔ `hal_Ipc_cm.*`

### Step 4: 임시 파일 삭제 및 뒷정리
작업 과정 중 백업 혹은 비교 목적으로 생성된 임시 파일이 있다면 프로젝트 폴더에서 모두 지웁니다.

### Step 5: 사용자 검토 및 빌드 요청
모든 수정 및 파일 이름 변경 작업이 끝나면, 빌드 시스템에 혼란이 없는지 점검하고 사용자에게 CCS(Code Composer Studio) IDE를 통해 수동으로 Rebuild를 요청합니다.

---

## 3. 사용자 승인 대기 (User Review Required)
> [!IMPORTANT]
> - 이 계획에 따라 C2000 및 ARM 펌웨어 코드의 일괄 수정 및 파일명 변경 작업이 수행됩니다.
> - 프로젝트 내비게이터(CCS) 상에서 파일이 변경된 것으로 인식될 수 있습니다.
> - 계획 내용에 추가하거나 변경하고 싶은 제약조건(메모)이 있다면 이 문서(`plan.md`)에 메모를 남겨주시거나 채팅으로 말씀해 주십시오.

이견이 없으시다면 **"구현 시작해줘"**라고 지시해 주시기 바랍니다. 승인 즉시 Step 1부터 순차적으로 코드를 수정하겠습니다.
