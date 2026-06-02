# 📋 ADC 온도 센서 전용 ePWM9 신설 및 이더넷 PHY 리셋 해제(GPIO 147 교정) 구현 계획서 (Plan)

**작성 일자**: 2026. 06. 02.  
**대상 파일**: 
1. [DevAdc.h](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/Dev/DevAdc.h) [MODIFY] - 완료
2. [DevAdc.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/Dev/DevAdc.c) [MODIFY] - 완료
3. [DevDspInit.c](file:///d:/Nexcom/Firmware/01_Project/02_Tester/TMDSCNCD28388D_T/TMDSCNCD28388D_T/TMDSCNCD28388D_T_CPU1/Dev/DevDspInit.c) [MODIFY]
**대상 코어**: CPU1 (C28x DSP Core)  

---

## 1. 개요 및 설계 사양 (Concept & Design Specification)

### 1.1 물리 이더넷 락업 해결 (PHY 하드웨어 핀 번호 교정: GPIO 147)
- **원인 교정**: TI TMDSCNCD28388D 컨트롤카드 보드 도면을 정밀 교차 추적한 결과, 온보드 이더넷 PHY 칩(DP83822)의 실제 하드웨어 리셋 핀(PHY_RST)은 GPIO 135번이 아닌 **`GPIO 147`**번 핀에 직결되어 있음이 확인되었습니다.
- 이전 패치에서 엉뚱한 135번 핀을 제어했기 때문에 PHY 칩이 여전히 깨어나지 못해 LED 불이 켜지지 않았던 것입니다.
- **해결책**: 리셋 제어 대상 핀을 **GPIO 147**번 핀으로 확실히 변경 조율하여 초기화 단계에서 리셋 펄스 방출 및 10ms 딜레이 후 정상 기동시킵니다.

---

## 2. 상세 수정 계획 (Proposed Changes)

### 2.1 [DevDspInit.c] [MODIFY]

#### ① `initEmacGpioPins` 함수 내에 이더넷 PHY 하드웨어 리셋 제어를 GPIO 147로 정밀 변경
- **기존 (Line 277~286)**:
```c
    /* --- 이더넷 PHY 칩(DP83822) 하드웨어 리셋 핀(GPIO 135) 활성화 및 리셋 해제 (물리 링크 기동용) --- */
    GPIO_setDirectionMode(135, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(135, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_135_GPIO135);
    
    // Active-Low 리셋 신호 방출 (강제 리셋 후 10ms 대기 후 해제하여 PHY 구동)
    GPIO_writePin(135, 0); 
    DEVICE_DELAY_US(10000); 
    GPIO_writePin(135, 1); 
```
- **변경안**:
```c
    /* --- 이더넷 PHY 칩(DP83822) 하드웨어 리셋 핀(GPIO 147) 활성화 및 리셋 해제 (물리 링크 기동용) --- */
    GPIO_setDirectionMode(147, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(147, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_147_GPIO147);
    
    // Active-Low 리셋 신호 방출 (강제 리셋 후 10ms 대기 후 해제하여 PHY 구동)
    GPIO_writePin(147, 0); 
    DEVICE_DELAY_US(10000); 
    GPIO_writePin(147, 1); 
```

---

## 3. 검증 계획 (Verification Plan)
1. 컴파일 완료 후 기판을 구동하였을 때, **RJ45 포트에 녹색 및 황색 LED 불이 켜지며 실시간으로 통신 링크가 잡히는지** 육안 모니터링합니다.
