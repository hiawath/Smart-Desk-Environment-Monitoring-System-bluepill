# Phase 1 리팩토링 보고서: 하드웨어 버그 수정 및 DWT 마이크로초 딜레이 통합

## 1. 개요
Phase 1에서는 컴파일러 최적화 옵션(-O0, -O2 등)에 따라 동작이 달라지던 불안정한 NOP 지연 루프를 제거하고, 하드웨어 사이클 카운터(Cortex-M3 DWT)를 기반으로 하는 정밀 마이크로초 지연 시스템을 구축하였습니다. 또한 시스템 오작동을 유발할 수 있는 하드웨어 설정 충돌 및 널 포인터 역참조 버그를 수정하였습니다.

---

## 2. 주요 변경 사항

### 2.1 공용 DWT 딜레이 모듈 구현 (`bsp_delay.c / h`)
- **문제점**: `bsp_delay.h` 헤더만 선언되어 있고 본체(`.c`)가 누락되어, 드라이버들(`bsp_dht11`, `bsp_hcsr04`, `bsp_ds1302`)이 각자 컴파일러 종속적인 NOP 루프(`us * 8`)를 중복 구현하여 사용함.
- **개선**:
  - Cortex-M3 DWT 사이클 카운터(`DWT->CYCCNT`) 기반 `delayInit()`, `delayUs()` 구현.
  - `delayGetCycles()`, `delayCyclesToUs()` 인라인 함수를 활용하여 시간 측정 신뢰성 확보.
  - `appInit.c`의 `bspInit()` 첫 단계에서 `delayInit()` 1회 호출.

### 2.2 HC-SR04 초음파 드라이버 버그 수정 및 DWT 측정 적용 (`bsp_hcsr04.c`)
- **문제점**:
  - `hcSr04Init()`에서 `pins == NULL`인 경우 `else` 분기에서 `pins->trig_port`를 역참조하는 잠재적 널 포인터 버그 존재.
  - Echo 펄스 측정 시 `volatile uint32_t c = 34` NOP 루프 카운팅으로 거리를 계산하여 Release 최적화 빌드 시 거리가 크게 왜곡됨.
- **개선**:
  - `hcSr04Init()`의 인자 유효성 검사 강화 (`if (!hhc || !pins) return;`).
  - Echo 핀의 HIGH 지속 시간을 `delayGetCycles()` 차이와 `delayCyclesToUs()`로 마이크로초 환산하여 거리 계산(`dist = duration_us / 58.0f`).

### 2.3 DHT11 및 DS1302 지연 루프 통합 (`bsp_dht11.c`, `bsp_ds1302.c`)
- **개선**:
  - 각 드라이버 내부의 로컬 NOP `delayUs()`를 제거하고 공용 `bsp_delay.h`로 통일.
  - DHT11의 40비트 데이터 수신 시 비트 판별(HIGH 지속 시간)을 DWT 사이클 차이 기반으로 정밀 판정.

### 2.4 타이머 주파수 통일 및 BGR LED 제어 안정화 (`tim.c`, `bsp_timer.c`)
- **문제점**:
  - TIM2의 Period가 65535(약 15Hz)로 설정되어 TIM3(Period 9999, 약 100Hz)와 불일치하며 B/G LED에서 육안 플리커 발생.
  - CubeMX에서 `BGR_GND_Pin`(PB0)이 Input으로 잡혀 있어 `timerInit()`에서 `WritePin(RESET)`을 호출해도 Floating 상태로 남음.
  - 실제 Cortex-M3 F103에는 없는 "TIM8" 주석이 산재함.
- **개선**:
  - TIM2의 `Period`를 `10000-1`(9999)로 통일하여 약 100Hz PWM으로 플리커 제거.
  - `timerInit()`에서 `BGR_GND_Pin`(PB0)을 `GPIO_MODE_OUTPUT_PP`로 명시적 초기화하고 GND를 구동하도록 수정.
  - TIM8 주석 및 잘못된 채널 설명을 TIM2 CH3/CH4로 수정.

### 2.5 미사용/위험 EXTI 콜백 제거 (`bsp_gpio.c`)
- **문제점**:
  - F1 HAL에 존재하지 않는 `HAL_GPIO_EXTI_Falling_Callback / Rising_Callback`이 구현되어 있어 호출되지 않는 데드 코드임.
  - `GPIO_PIN_15`는 `DHT11_Pin`(PB15)과 핀 번호가 겹쳐 향후 EXTI 활성화 시 DHT11 데이터 통신이 버튼 인터럽트로 오인될 위험이 있었음.
- **개선**: 비표준 및 잠재적 충돌 위험이 있는 미사용 EXTI 콜백 완전 제거.

### 2.6 UART 드라이버 정리 (`bsp_uart.c`)
- **개선**:
  - 미사용 `rx_data` 및 의미 없는 `'a'` 문자 검사, "Cortex-M4" 오타 문자열 제거.
  - DMA IDLE 수신 인터럽트(`HAL_UARTEx_RxEventCallback`)로 이벤트 수신 처리 정리.
  - 에러 콜백(`HAL_UART_ErrorCallback`)에서 `HAL_UARTEx_ReceiveToIdle_DMA`로 안정적인 재시작 보장.

---

## 3. 검증 결과
- **컴파일러**: ARM Clang (AC6 V6.24.0), MicroLib
- **빌드 결과**: 1 succeeded, 0 failed
- **프로그램 크기**: Code=37964, RO-data=1968, RW-data=168, ZI-data=6520 (기존 38184 대비 220바이트 절감)
