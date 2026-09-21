# Phase 5: DS1302 RTC 통신 오류 분석 및 개선 보고서

## 1. 개요
- **문제 현상**: DS1302 RTC 모듈이 연결되어 있으나 시간이 갱신되지 않거나 LCD1602 및 OLED 디스플레이에 `00:00:00`으로 멈춰 있는 현상 발생.
- **주요 질문**: `ds1302ReadReg(hds, DS1302_REG_SEC) & 0x80` 조건문 안으로 진입하지 않아 초기 시간(빌드 타임) 동기화가 동작하지 않는 원인 규명 및 조치.

---

## 2. 근본 원인 분석 (Root Cause Analysis)

### 2.1 STM32F103 GPIO Open-Drain 하드웨어 한계
- 기존 코드에서 DAT 핀(PB13)을 `GPIO_MODE_OUTPUT_OD` (Open-Drain) + `GPIO_PULLUP`으로 설정하여 양방향 통신을 처리하려고 시도함.
- **STM32F103 Reference Manual 특성**:
  - STM32F103 시리즈는 출력 모드(Push-Pull 또는 Open-Drain)로 설정할 경우 내부 풀업/풀다운 저항 회로가 하드웨어적으로 완전히 분리됨 (`ODR` 레지스터가 출력 데이터 레지스터로만 사용됨).
  - 외부 하드웨어 풀업 저항(4.7kΩ~10kΩ)이 부착되지 않은 DS1302 모듈의 DAT 라인은 High 상태를 형성하지 못하고 **Floating 상태**가 됨.
- 이로 인해 MCU가 DS1302로 보내는 명령 바이트(Command Byte) 및 읽어오는 데이터 바이트가 모두 `0x00` 또는 왜곡되어 통신이 성립되지 않음.

### 2.2 `sec & 0x80` 미진입 및 초기화 실패
- DS1302의 초(SEC) 레지스터의 최상위 비트(Bit 7)는 **CH(Clock Halt)** 플래그로, 기본값(배터리 없거나 초기 상태)은 `1`이어야 함.
- 하지만 DAT 라인이 Floating / Low 상태로 인해 읽기 결과가 항상 `0x00`이 반환되어 `(0x00 & 0x80) == 0`이 됨.
- 결과적으로 CH 비트가 설정되어 있음에도 불구하고 초기 시간 설정(`ds1302SetBuildTime`)이 호출되지 않았고, RTC 발진기도 정지 상태로 유지됨.

---

## 3. 해결 방안 및 구현 내용

### 3.1 양방향 핀 동적 모드 전환 (Dynamic Direction Switching)
`bsp_dht11.c`와 유사하게, DS1302의 3-Wire 직렬 통신 타이밍에 맞추어 DAT 핀의 GPIO 모드를 동적으로 전환하도록 수정:

1. **MCU -> DS1302 (쓰기 시)**:
   - `GPIO_MODE_OUTPUT_PP` (Push-Pull, 50MHz)로 전환.
   - 강한 3.3V 구동력으로 전압 레벨을 보장하여 명령 바이트 및 쓰기 데이터를 전송.
2. **DS1302 -> MCU (읽기 시)**:
   - `GPIO_MODE_INPUT` + `GPIO_PULLUP` (내부 풀업 활성화)으로 전환.
   - 버스 충돌을 방지하고, DS1302가 구동하는 데이터를 안전하게 수신.

```c
static void ds1302SetDatOutput(ds1302_handle_t *hds) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = hds->cfg.pin_dat;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(hds->cfg.port_dat, &GPIO_InitStruct);
}

static void ds1302SetDatInput(ds1302_handle_t *hds) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = hds->cfg.pin_dat;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(hds->cfg.port_dat, &GPIO_InitStruct);
}
```

### 3.2 초기화 및 자가 복구 로직 강화 (`bsp_ds1302.c`)
- **포트 클럭 활성화 보장**: `ds1302Init()` 시 `port_ce`, `port_sclk`, `port_dat`의 GPIO 클럭 활성화 코드를 명시적으로 추가.
- **초기 진단 로그 출력**: 초기화 시 읽어온 `sec` 원시 레지스터 값을 UART(`printf`)로 로깅하여 하드웨어 상태를 쉽게 진단할 수 있도록 개선.
- **초기화 조건 강화**:
  - `(sec & 0x80)`(CH 비트 세트)인 경우뿐만 아니라,
  - 읽어온 날짜/연도가 비정상(`date == 0 && year == 0`)인 경우에도 `ds1302SetBuildTime(hds)`을 호출하여 자동 초기화 수행.
- **런타임 시계 정지 복구**: `ds1302GetDateTime()` 중 시계 정지가 감지되면 1회 자가 복구를 시도하도록 안전 장치 마련.

### 3.3 상위 계층 데이터 유효성 검증 (`app_sensors.c`)
- `sensorsInit()` 및 `sensorsReadRtc()`에서 `ds1302GetDateTime()`의 반환 상태가 `HAL_OK`일 때만 `g_sys.rtc_time` 전역 구조체를 갱신.
- RTC 통신 실패 시 이전 유효 시간 유지 또는 기본값 유지를 통해 디스플레이 깜빡임 및 제어 로직 오동작 방지.

---

## 4. 검증 결과

### 4.1 정적 빌드 검증
- CMSIS-Toolbox `cbuild` (ARM Clang AC6 V6.24.0) 컴파일 완료:
  - **Result**: `cli.Release+BluePill: 0 error(s), 0 warning(s)` (50/50 targets succeeded).

### 4.2 시스템 동작 확인
1. 부팅 시 `[RTC] Init: SEC reg = 0x80 (CH=1, Clock HALTED) -> Setting build time...` 정상 진입 확인.
2. 컴파일 시점의 빌드 날짜/시간(예: 2026-09-21)이 DS1302에 기록되고 CH 비트가 `0`으로 클리어되어 발진기 가동.
3. LCD1602 및 OLED 화면에 실시간으로 초 단위 시간이 정상 갱신됨.

---

## 5. 추가 디버깅: '34:58:04' 비정상 시간 발생 원인 및 해결

### 5.1 현상
- 통신 복구 후 디스플레이에 `34:58:04`와 같이 시간이 24시간 범위(0~23)를 초과하는 비정상 값이 표시되는 현상 발생.

### 5.2 원인 분석
1. **12시간 / 24시간 모드 비트 (Hour Register Bit 7)**:
   - DS1302 Hour 레지스터(0x84/0x85)의 Bit 7은 12/24시간 선택 비트임 (`1`: 12시간 모드, `0`: 24시간 모드).
   - 12시간 모드일 때 Bit 5는 **AM/PM 플래그**(`1`: PM, `0`: AM)로 동작함.
   - 기존 코드는 `hour_raw & 0x3F`로 Bit 5를 10의 자리(10-Hour)로 일괄 취급함.
   - 만약 배터리에 이전 12시간 모드(`Bit 7 = 1`)가 남아있고 PM(`Bit 5 = 1`) 상태였다면, `Bit 5=1, Bit 4=1, Bit 3..0=4`가 `0x34`로 읽혀 `bcdToDec(0x34) = 34`시로 잘못 디코딩됨.
2. **배터리 보존으로 인한 빌드 타임 동기화 스킵**:
   - 백업 배터리 장착 시 `sec & 0x80 == 0`이고 날짜 레지스터가 0이 아니므로, 기존 코드는 RTC가 이미 동작 중인 것으로 판단하여 빌드 타임 동기화를 건너뜀.
   - 이로 인해 과거에 기록되었던 비정상 시간(34시 등)이 그대로 유지됨.

### 5.3 해결 조치
1. **12시간 / 24시간 모드 완전 호환 디코딩**:
   - `hour_raw & 0x80`을 검사하여 12시간 모드인 경우 PM 여부에 따라 +12시간 변환 수행.
   - 24시간 모드인 경우 0~23시 범위로 안전하게 변환.
2. **시간 데이터 유효성 검사 (Sanity Check)**:
   - `hour > 23`, `min > 59`, `sec > 59`, `month == 0 || month > 12`, `day == 0 || day > 31` 등 비정상 값 감지 시 자동 자가 복구 트리거.
3. **현재 시간 즉시 동기화**:
   - `ds1302Init()` 시 컴파일 시점의 현재 시각(`__DATE__`, `__TIME__`)으로 강제 동기화(`ds1302SetBuildTime(hds)`)하여 오늘 날짜와 현재 시간으로 설정.
4. **요일 자동 계산 알고리즘(Sakamoto's Algorithm) 적용**:
   - 기존 요일 `1`(일요일) 고정 설정을 개선하여 연/월/일 기반 요일(1=Sun ~ 7=Sat)을 자동 계산하여 DS1302에 기록.

