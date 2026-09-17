# Phase 1 시스템 종합 검증 및 시험 성적서

## 1. 개요
본 문서는 **Smart Desk Environment Monitoring System (Phase 1)**의 하드웨어/소프트웨어 통합 구현 완료 후, 빌드 무결성, 리소스 점유율 및 6대 핵심 기능 검증 결과를 종합 보고합니다.

---

## 2. 펌웨어 빌드 환경 및 리소스 점유율

### 2.1 빌드 툴체인
- **Target MCU**: STM32F103C8T6 (Arm Cortex-M3, 72MHz)
- **Compiler**: Arm Compiler for Embedded (AC6) v6.24.0
- **Build System**: Arm CMSIS-Toolbox 2.14.1 + CMake 4.3.3 + Ninja 1.13.2
- **C Runtime**: Arm MicroLib (`--library_type=microlib`)

### 2.2 메모리 사용량 (Memory Footprint)
```
============================================================
Program Size Summary:
  Code (Flash Executable)       :  38,184 bytes
  RO-data (Constants/Fonts)     :   2,004 bytes
  RW-data (Initialized RAM)     :     168 bytes
  ZI-data (Zero-init BSS RAM)   :   6,520 bytes
============================================================
Flash Memory Usage : 40,188 bytes (~61.3% of 64KB Flash)
SRAM Memory Usage  :  6,688 bytes (~32.6% of 20KB SRAM)
============================================================
Build Result       : 1 succeeded, 0 failed (Total 41 units built)
Firmware Artifacts : Project.axf (ELF), Project.hex
============================================================
```

---

## 3. 기능별 검증 항목 및 시험 결과 (Checklist)

| 번호 | 기능 검증 항목 | 설계 요구사항 | 시험 및 검증 결과 | 판정 |
| :---: | :--- | :--- | :--- | :---: |
| **1** | **DS1302 RTC** | - 시/분/초 정확한 유지<br>- 배터리 부재 시 빌드 타임 자동 동기화 | - CH(Clock Halt) 비트 검사 후 컴파일 시간 자동 주입 확인<br>- 1초 주기로 시간 정상 카운트 및 표시 | **PASS** |
| **2** | **DHT11 환경 센서** | - 2초 주기 실내 온도(℃) 및 습도(%) 측정 | - 72MHz 기준 1-Wire 펄스 타이밍 보정 완료<br>- 정상 범위 온습도 데이터 획득 | **PASS** |
| **3** | **HC-SR04 거리 센싱** | - 100ms 고속 거리 측정<br>- 센서 글리치 제거를 위한 EMA 필터 | - 10µs 트리거 펄스 및 에코 수신 정상<br>- $\alpha=0.7$ EMA 필터로 부드러운 거리값 산출 | **PASS** |
| **4** | **STM32 내부 온도** | - ADC1 DMA 주기적 샘플링 및 온도 변환 | - 500ms 주기 DMA 변환 및 공식 기반 ℃ 연산 정상 동작 | **PASS** |
| **5** | **듀얼 디스플레이 UI** | - SSD1306(SPI): 타이틀, 시계, 센서값, 바 게이지, 스피너<br>- LCD1602(I2C): 16x2 텍스트 및 백라이트 제어 | - Bit-Banging SPI로 OLED 초고속 렌더링 확인<br>- LCD1602 2열 정보 가독성 높게 표시 확인 | **PASS** |
| **6** | **스마트 파워 시스템** | - 10초 이상 미감지 시 자동 절전<br>- 2~60cm 착석 감지 시 즉각 화면 기상 | - 사용자 부재 10초 후 LCD 백라이트 소등 & OLED 화면 클리어<br>- 근접 시 즉시 화면 복구 및 정상 표시 재개 | **PASS** |

---

## 4. 펌웨어 다운로드 및 실행 가이드

### 4.1 ST-Link를 통한 플래싱 (pyOCD)
VS Code의 Run/Tasks 기능 또는 터미널 명령어를 통해 Blue Pill 보드에 바이너리를 즉시 기록할 수 있습니다:
```powershell
pyocd load --probe stlink: --chip stm32f103c8 out/Project/STM32F103C8/Debug/Project.hex
```

### 4.2 UART 텔레메트리 모니터링
- 포트: ST-Link Virtual COM Port (PA2-TX, PA3-RX)
- 속도: 115200 bps (8-N-1)
- 출력 예시:
  ```text
  [11:42:00] T:24.0C, H:50.0%, D:28.4cm, McuT:31.2C [Screen:ON]
  [11:42:01] T:24.0C, H:50.0%, D:28.3cm, McuT:31.3C [Screen:ON]
  ...
  [POWER] User absent for 10 sec -> Display OFF (Sleep)
  ...
  [POWER] User detected (29.1cm) -> Display ON
  ```

---

## 5. 결론
`DOC/Phase1_System_Design.md`의 모든 요구사항(Time Keeping, Environment Sensing, Proximity Sensing, Dual Display, Smart Power, Non-blocking Scheduler)이 성공적으로 구현 및 검증 완료되었습니다.
