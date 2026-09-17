# Phase 1 하드웨어 핀 배선 및 BSP 드라이버 설계서

## 1. 개요
본 문서는 **Smart Desk Environment Monitoring System (Phase 1)** 구현을 위한 STM32F103 (Blue Pill) 마이크로컨트롤러와 센서 및 디스플레이 모듈 간의 하드웨어 핀 연결 사양, 타이밍 보정 내역 및 BSP 드라이버 아키텍처를 정의합니다.

---

## 2. 하드웨어 핀 매핑 (STM32F103C8T6 Blue Pill)

| 장치 구분 | 신호 명칭 | MCU 핀 | GPIO 모드 | 내부 풀업/다운 | 설명 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **DS1302 RTC** | RST (CE) | `PB12` | Output Push-Pull | No Pull | 칩 활성화 신호 (High Active) |
| | DAT (I/O) | `PB13` | Bi-directional | Pull-up | 3-Wire 양방향 시리얼 데이터 |
| | CLK (SCLK) | `PB14` | Output Push-Pull | No Pull | 시리얼 통신 클럭 |
| **DHT11 온습도** | DATA | `PB15` | Bi-directional | Pull-up | 1-Wire 단선 양방향 데이터 |
| **HC-SR04 초음파** | TRIG | `PA8` | Output Push-Pull | No Pull | 10µs 거리 측정 트리거 펄스 |
| | ECHO | `PA9` | Input | No Pull | 에코 펄스 수신 (High 지속시간 측정) |
| **SSD1306 OLED**<br>(7-Pin SPI) | CS | `PA4` | Output Push-Pull | No Pull | Chip Select (Low Active) |
| | D0 (SCK) | `PA5` | Output Push-Pull | No Pull | SPI 직렬 클럭 (Bit-Banging) |
| | DC | `PA6` | Output Push-Pull | No Pull | Data(High) / Command(Low) 제어 |
| | D1 (MOSI) | `PA7` | Output Push-Pull | No Pull | SPI 데이터 출력 |
| | RES | `PA1` | Output Push-Pull | No Pull | 하드웨어 리셋 펄스 (Low Active) |
| **LCD1602 LCD**<br>(PCF8574 I2C) | SCL | `PB6` | Alternate Function OD | External 4.7k | I2C1 클럭 신호 |
| | SDA | `PB7` | Alternate Function OD | External 4.7k | I2C1 데이터 신호 |
| **온보드 LED** | LD2 | `PC13` | Output Open-Drain | No Pull | 시스템 생존/동작 인디케이터 (Active Low) |
| **UART 디버그** | TX / RX | `PA2` / `PA3` | USART2 (AF) | - | 115200bps 디버그 텔레메트리 출력 |
| **ADC 측정** | Internal | ADC1_IN16 | Analog In | - | MCU 내부 온도 센서 측정 (DMA) |

---

## 3. BSP 타이밍 보정 및 드라이버 최적화

### 3.1 마이크로초(µs) 딜레이 최적화
- 기존 코드는 Cortex-M33 250MHz 프로세서 기준(`count = us * 42`)으로 작성되어 있어, 72MHz로 구동되는 STM32F103 환경에서 딜레이가 약 3.5배 과도하게 연장되는 문제가 있었습니다.
- Cortex-M3 72MHz 실행 환경에 맞추어 `volatile uint32_t count = us * 8`로 보정하여 1-Wire(DHT11) 및 초음파(HC-SR04) 펄스 타이밍 규격을 정확하게 만족하도록 수정하였습니다.

### 3.2 고속 Bit-Banging SPI 디스플레이 구동
- CubeMX에서 하드웨어 SPI1 활성화가 누락되어 있던 문제를 해결하기 위해, 소프트웨어 고속 Bit-Banging SPI 전송 방식을 구현하였습니다.
- 별도의 외부 라이브러리나 CubeMX 설정 재생성 없이 GPIO 레지스터 직접 제어를 통해 최대 수 MHz 상당의 고속 화면 갱신이 가능하도록 설계되었습니다.

---

## 4. 빌드 및 동작 검증
- 컴파일러: ARM Compiler for Embedded (armclang AC6 v6.24.0)
- CMSIS Toolbox cbuild 빌드 성공:
  - 출력 파일: `out/Project/STM32F103C8/Debug/Project.axf`
  - 프로그램 크기: Code=37,888 bytes, RO-data=1,840 bytes, RW-data=168 bytes, ZI-data=6,520 bytes.
