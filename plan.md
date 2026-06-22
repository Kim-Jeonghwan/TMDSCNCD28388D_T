# C# PC 프로그램 빌드 경고 해결 계획서

## 1. 개요
PC 모니터링 프로그램(`TMDSCNCD28388D_T_PC`)에서 C# Nullable Reference Types 활성화로 인해 발생하는 빌드 경고(Warning: CS8618, CS8600, CS8602, CS8604, CS8622, CS8625)를 해결하여, 빌드 경고가 없는 안전하고 깔끔한 코드베이스를 구축하기 위한 계획서입니다.

## 2. Open Questions & User Review
> [!IMPORTANT]
> - 본 수정은 빌드 시 발생하는 경고를 제거하기 위한 C# Nullable Reference Types 전용 문법 수정입니다.
> - 기존 프로그램의 동작 로직(비즈니스 로직)은 100% 동일하게 유지됩니다.
> - 수정 후 한글 주석의 인코딩이 깨지지 않도록 **UTF-8 인코딩**을 엄격히 준수하여 적용하겠습니다.
> - 에이전트 수칙에 따라 실제 구현은 이 계획에 대해 사용자의 명시적인 승인(예: "승인합니다", "구현 시작해줘")을 받은 뒤에 시작합니다.

## 3. 대상 파일 및 Proposed Changes

---

### [TMDSCNCD28388D_T_PC/CanProtocol.cs](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_PC/CanProtocol.cs)
- **[MODIFY]** `_serialPort`, `_readThread` 필드를 nullable(`SerialPort?`, `Thread?`)로 선언하여 생성자 미초기화 경고(CS8618) 해결.
- **[MODIFY]** 이벤트 `OnStatusReceived`, `OnCommError`, `OnPortClosed`, `OnRawTx`, `OnRawRx`에 `?`를 붙여 nullable 이벤트(`Action<...>?`)로 선언(CS8618 해결).

---

### [TMDSCNCD28388D_T_PC/UdpEthProtocol.cs](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_PC/UdpEthProtocol.cs)
- **[MODIFY]** `_udpClient`, `_dspEndPoint`, `_localEndPoint`, `_rxThread` 필드를 nullable(`UdpClient?`, `IPEndPoint?`, `Thread?`)로 선언(CS8618 해결).
- **[MODIFY]** 이벤트 `OnStatusReceived`, `OnCommError`, `OnPortClosed`, `OnRawTx`, `OnRawRx`에 `?`를 붙여 nullable 이벤트로 선언(CS8618 해결).
- **[MODIFY]** `Disconnect`에서 `_udpClient = null;`이 경고 없이 대입되도록 처리(CS8625 해결).
- **[MODIFY]** `ReceiveWorker` 내 `_udpClient?.Receive(ref remoteEp)`의 반환값을 받는 로컬 변수를 `byte[]? data`로 지정하여 널 변환 경고 해결(CS8600 해결).

---

### [TMDSCNCD28388D_T_PC/LogForm.cs](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_PC/LogForm.cs)
- **[MODIFY]** `FlushLogs` 내부의 `while (_logQueue.TryDequeue(out string logLine))` 선언을 `out string? logLine`으로 수정하여 널 반환 형식 경고 해결(CS8600 해결).

---

### [TMDSCNCD28388D_T_PC/MainForm.cs](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_PC/MainForm.cs)
- **[MODIFY]** WinForms UI 컨트롤 필드들(`cmbPorts`, `cmbBauds`, `btnConnect` 등) 및 `_timer`, `_reqTimer` 필드 선언 시 `= null!;`를 추가하여 생성자 미초기화 경고(CS8618) 해결.
- **[MODIFY]** `Timer_Tick(object sender, EventArgs e)` 및 `ReqTimer_Tick(object sender, EventArgs e)`의 첫 번째 매개변수를 `object? sender`로 변경하여 `EventHandler`와 델리게이트 nullability 일치(CS8622 해결).
- **[MODIFY]** `cmbWaveType.SelectedItem`, `cmbPorts.SelectedItem`, `p["Caption"]` 등을 안전하게 문자열 변환 및 파싱하도록 `?` 연산자와 널 병합 연산자(`??`) 등을 추가하여 역참조 오류(CS8602, CS8600, CS8604) 해결.

---

## 4. 검증 계획
1. **[ ] 정합성 검사**: 수정 완료 후 `view_file`을 통해 수정한 소스 파일의 코드가 깨지지 않았는지, 한글 주석이 깨짐 없이 잘 유지되는지 검증합니다.
2. **[ ] 빌드 경고 검증 요청**: 사용자에게 C# PC 프로젝트를 빌드하도록 요청하여 빌드 출력에서 관련된 Nullable Reference Types 관련 Warning이 완전히 사라졌는지 확인합니다.
