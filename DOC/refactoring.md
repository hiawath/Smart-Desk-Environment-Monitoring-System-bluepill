## 프로젝트 분석

### 개요
| 항목 | 내용 |
|---|---|
| 타겟 | STM32F103C8T6 (Blue Pill), Cortex-M3 72MHz |
| 빌드 | CMSIS-Toolbox csolution (AC6 + MicroLib), Debug(`-O0`) / Release(`balanced`) |
| 프레임워크 | STM32 HAL (CubeMX 생성 `Core/`) + 자체 `MyApp/` |
| 기능 | Smart Desk Monitor — DS1302 RTC, DHT11, HC‑SR04, SSD1306 OLED(SPI bit‑bang), LCD1602(I2C), 내부 ADC 온도, RGB LED Breathing PWM, UART 텔레메트리, 초음파 기반 자동 절전 |

### 계층 구조
```
main.c (CubeMX) ─► appInit() ─► bspInit()  : 드라이버 핸들 생성/초기화
                └► appMain()               : 무한루프 스케줄러 + 상태머신 + UI + 로그 (전부 한 함수)
MyApp/bsp/ : dht11, hcsr04, ds1302, lcd1602, ssd1306(I2C), ssd1306_spi, adc, timer, uart, i2c, gpio
```
설계 문서(DOC/)가 잘 정리되어 있고, 비차단 스케줄링·EMA 필터·상태머신 등 의도는 명확합니다. 다만 코드가 문서의 구조를 따라가지 못하고 `appMain()` 하나에 집중되어 있습니다.

---

## 발견된 문제점

### 🔴 버그 / 동작 위험 (우선 수정)

1. **`hcSr04Init()` NULL 역참조** — `bsp_hcsr04.c`
   ```c
   if (pins) { hhc->pins = *pins; }
   else { hhc->pins.trig_port = pins->trig_port; ... }  // pins == NULL 인데 역참조
   ```

2. **NOP 루프 기반 `delayUs()`가 최적화 레벨에 종속** — `bsp_dht11.c`, `bsp_hcsr04.c`, `bsp_ds1302.c` 3곳에 복붙되어 있고, `us * 8` 계수와 HC‑SR04의 `c = 34` 튠값은 `-O0` 기준입니다. Release(`balanced`)로 빌드하면 DHT11/HC‑SR04 타이밍이 깨질 가능성이 매우 높습니다.

3. **TIM2 PWM 주파수 불일치 → LED 깜빡임** — `tim.c`에서 TIM2 `Period = 65535`(≈15 Hz), TIM3 `Period = 9999`(≈100 Hz). TIM2에 연결된 B/G LED는 눈에 보이는 플리커가 발생합니다. 또 `bsp_timer.c` 주석은 전부 "TIM8"인데 F103C8에는 TIM8이 없고 코드는 TIM2를 사용합니다(오래된 주석).

4. **`BGR_GND_Pin`이 INPUT 모드** — `gpio.c(MX_GPIO_Init)`에서 `BGR_GND|BGR_G|BGR_B`를 `GPIO_MODE_INPUT`으로 설정. 이후 `timerInit()`이 G/B만 AF_PP로 바꾸고, GND 핀은 입력 상태로 남은 채 `WritePin(RESET)`을 호출합니다(효과 없음).

5. **F1 HAL에 존재하지 않는 콜백** — `bsp_gpio.c`의 `HAL_GPIO_EXTI_Falling_Callback / Rising_Callback`은 G0/H5/U5 계열 API입니다. F1은 `HAL_GPIO_EXTI_Callback(uint16_t)` 하나뿐이라 **절대 호출되지 않는 데드 코드**입니다. 게다가 EXTI 자체가 CubeMX에 설정되어 있지 않고, Rising 콜백은 `PA5`(= SSD1306_SCK)를 토글합니다.

6. **`bsp_uart.c` 수신 로직** — `rx_data`는 어디서도 쓰이지 않는데 `'a'`와 비교, `RxCpltCallback`/`RxEventCallback` 내용 중복, 에러 콜백은 `Receive_DMA`(IDLE 아님)로 재시작, 메시지에 "Cortex‑M4".

7. **CubeMX와 드라이버의 GPIO 설정 충돌** — SSD1306/HC‑SR04/DS1302/DHT11/타이머가 각자 `__HAL_RCC_GPIOx_CLK_ENABLE()` + `HAL_GPIO_Init()`을 호출하고, `main.h`의 핀 정의는 `.ioc`에 반영되어 있지 않습니다. CubeMX 재생성 시 상태가 달라질 수 있습니다.

### 🟠 구조적 문제

8. **`appMain()` God Function (~200줄)** — 스케줄러, 4개 센서 읽기, 스마트파워 상태머신, OLED 렌더링, LCD 렌더링, UART 로그, 함수 내부 `#define`까지 한 곳에. OLED 프레임/타이틀 그리는 코드가 부팅 시와 Wake‑up 시 2번 복붙됨.

9. **SSD1306 코드 이중화** — `bsp_ssd1306.c`(I2C)와 `bsp_ssd1306_spi.c`(SPI)가 폰트 테이블(≈95줄), 그래픽 프리미티브(Pixel/Line/Rect/Char/String) 전부 복제. I2C 버전은 `cproject.yml`에 포함되지도 않는 **데드 파일**인데 `appInit.h`에서 헤더는 include합니다. 두 폰트의 `g/j/p/q/y` 글리프도 서로 다릅니다.

10. **일관성 없는 "객체지향" 패턴** — 드라이버마다 방식이 다릅니다.
    | 드라이버 | 방식 |
    |---|---|
    | dht11 / hcsr04 / lcd1602 | 핸들에 함수포인터 + `s_active_xxx` 전역 싱글턴 (→ 다중 인스턴스 불가, 함수포인터는 RAM만 소비) |
    | ssd1306_spi | 상수 초기화된 전역 핸들 + 함수포인터 |
    | ds1302 / adc / timer / uart | 일반 함수 |
    
    `hlcd1602.printf`와 `lcd1602Printf()`처럼 같은 본문이 두 번 존재하는 것도 이 패턴의 부산물입니다.

11. **이중 스케줄링** — `adcUpdate()`가 내부적으로 500ms 주기를 갖고, `appMain`도 500ms 태스크에서 `adcGetTemp()`를 호출합니다.

12. **사용되지 않는 코드** — `timerSetDutyFloat`, `s_current_duty`, `is_n_channel`, `bgrNextStep/bgrSetColor`(PWM 핀과 충돌), `SPI1_SS_Pin` alias, 빈 `i2cInit()`, `ssd1306SpiTest`, `SSD1306_SPI_COLOR_YELLOW/BLUE`(둘 다 1).

### 🟡 성능 / 품질

13. **블로킹 드라이버가 "비차단" 루프 안에 있음** — HC‑SR04는 100ms마다 최대 ~35ms 폴링, DHT11은 `HAL_Delay(18)` + ~5ms `__disable_irq()`(SysTick 손실 → `HAL_GetTick` 드리프트). OLED는 1024바이트를 `HAL_GPIO_WritePin` 3회/비트로 bit‑bang(≈25k HAL 호출/프레임, 10 fps).
14. PA5/PA7은 하드웨어 **SPI1** 핀인데 사용하지 않음.
15. UI 좌표·문자열 매직넘버 산재, 레이아웃 변경 시 두 곳 이상 수정 필요.
16. `main.c`에서 `APBPrescTable`을 직접 정의 — DFP의 `system_stm32f10x.c`와 HAL 버전 불일치를 우회한 흔적. 취약한 지점이므로 주석으로 이유를 남기거나 `Core/Src/system_stm32f1xx.c`를 사용하도록 정리 필요.
17. **`bsp_delay.h` 헤더만 존재하고 구현체(`.c`) 누락** — DWT 기반 지연 함수 선언과 인라인 함수는 정의되어 있으나, `delayInit()`과 `delayUs()` 구현 파일이 없어 각 센서 드라이버가 임의의 NOP 루프를 중복 구현하여 사용함.
18. **`bsp_gpio.c`의 EXTI15 핀과 `DHT11_Pin`(PB15) 충돌 위험** — F103에서 EXTI 라인 15는 모든 Port의 Pin 15가 공유함. 버튼 인터럽트 핸들러가 GPIO_PIN_15를 감시하도록 되어 있어, CubeMX에서 EXTI15를 활성화하면 DHT11 통신 시마다 버튼 눌림으로 오작동(Breathing 모드 전환) 발생 위험.
19. **ADC One-shot DMA와 CubeMX `ContinuousConvMode=ENABLE` 불일치** — 드라이버(`bsp_adc.c`)는 500ms마다 1회 변환 후 콜백에서 `HAL_ADC_Stop_DMA`를 호출하는 One-shot 방식인데, CubeMX 설정(`adc.c`)은 Continuous로 되어 있어 하드웨어 설정과 소프트웨어 의도가 불일치함.
20. **LCD1602 일시적 I2C 에러 시 영구 정지** — `bsp_lcd1602.c`의 `lcd1602_write`에서 전송 실패 시 `hlcd->initialized = false;`로 설정되어, 버스 노이즈 등으로 단 1회 실패하더라도 MCU 리셋 전까지 디스플레이가 영구히 먹통됨 (`i2cBusRecover()` 연계 필요).

---

## 리팩토링 추천

### 1단계 — 버그 수정 (동작 안정화)
- `hcSr04Init` else 분기 제거.
- **공용 `bsp_delay.c/h`** 생성, DWT 사이클 카운터 기반으로 교체 (최적화 레벨 무관):
  ```c
  // bsp_delay.c
  void delayInit(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
  }
  void delayUs(uint32_t us) {
    uint32_t start  = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < cycles) { }
  }
  ```
  HC‑SR04의 에코 측정도 `duration_count` 대신 `DWT->CYCCNT` 차이로 계산.
- CubeMX에서 TIM2 `Period = 9999`로 통일, PB0/PB10/PB11을 각각 Output/TIM2_CH3/TIM2_CH4로 설정 → `timerInit()`의 GPIO 코드 제거.
- `bsp_gpio.c`의 EXTI 콜백 삭제(또는 PA15 EXTI를 CubeMX에 추가하고 `HAL_GPIO_EXTI_Callback`으로 구현).
- `bsp_uart.c`: `rx_data` 제거, 콜백 하나로 정리, 에러 콜백은 `ReceiveToIdle_DMA`로 재시작.

### 2단계 — 구조 분리 (appMain 해체)

**제안 디렉터리**
```
MyApp/
  app/
    app_config.h      # SLEEP_TIMEOUT_MS, PRESENCE_DIST_*, EMA_ALPHA, 태스크 주기 등 상수 모음
    app_data.h        # sysData_t (센서 스냅샷 공유 구조체)
    app_scheduler.c/h # 주기 태스크 테이블
    app_sensors.c/h   # DHT11/RTC/HC-SR04/ADC 읽기 → sysData_t 갱신 (EMA 포함)
    app_power.c/h     # Screen ON/OFF 상태머신
    app_log.c/h       # UART 텔레메트리 포맷
    appMain.c         # 초기 화면 + 태스크 등록 + while(1){ schedulerRun(); }
  ui/
    ui_oled.c/h       # 프레임/타이틀/동적 영역 렌더링 (좌표 매크로화)
    ui_lcd.c/h        # 16x2 텍스트 레이아웃
  gfx/
    gfx.c/h           # 프레임버퍼 프리미티브 (Pixel/Line/Rect/Char/String) — 전송 계층 무관
    font6x8.h         # 폰트 테이블 단일화
  drivers/
    ssd1306.c/h       # init 시퀀스 + update(), 하위 port 인터페이스 사용
    ssd1306_port_spi.c# bit-bang 또는 HW SPI 전송 구현 (I2C 포트가 필요하면 별도 파일)
    dht11.c/h  hcsr04.c/h  ds1302.c/h  lcd1602.c/h
  bsp/
    bsp_delay.c/h  bsp_uart.c/h  bsp_adc.c/h  bsp_pwm_led.c/h(← bsp_timer)  bsp_i2c.c/h
```

**공유 데이터 모델**
```c
// app_data.h
typedef struct {
  ds1302Time_t rtc;
  float temp_c, humi_pct;   // DHT11
  float dist_cm;            // EMA 적용값
  float mcu_temp_c;         // 내부 ADC
  bool  dht_valid, dist_valid;
} sysData_t;

extern sysData_t g_sys;
```

**태스크 테이블 스케줄러**
```c
// app_scheduler.h
typedef struct {
  uint32_t period_ms;
  uint32_t last_tick;
  void   (*fn)(void);
} task_t;

void schedulerRun(task_t *tasks, size_t n);

// appMain.c
static task_t s_tasks[] = {
  { TASK_FAST_MS,   0, taskFast   },  // 100ms : 초음파 → EMA → power FSM → OLED
  { TASK_MID_MS,    0, taskMid    },  // 500ms : ADC 온도, LD2
  { TASK_SLOW_MS,   0, taskSlow   },  // 1000ms: RTC, LCD, 로그
  { TASK_ENV_MS,  500, taskEnv    },  // 2000ms: DHT11 (offset 500ms)
};

void appMain(void) {
  uiOledDrawFrame();
  uiLcdShowBoot();
  adcStartDMA(TASK_MID_MS);
  while (1) {
    timerLedUpdate();
    adcUpdate();
    schedulerRun(s_tasks, ARRAY_SIZE(s_tasks));
  }
}
```

**스마트 파워 상태머신 분리**
```c
// app_power.c
typedef enum { SCREEN_ON, SCREEN_OFF } screenState_t;

void powerUpdate(float dist_cm) {
  bool present = (dist_cm >= PRESENCE_DIST_MIN && dist_cm <= PRESENCE_DIST_MAX);
  uint32_t now = HAL_GetTick();

  if (present) {
    s_last_presence = now;
    if (s_state == SCREEN_OFF) powerWake(dist_cm);
  } else if (s_state == SCREEN_ON && now - s_last_presence >= SLEEP_TIMEOUT_MS) {
    powerSleep();
  }
}
bool powerIsScreenOn(void);
```
`powerWake()`/`powerSleep()`이 `uiOledDrawFrame()`, `uiLcdSetBacklight()`를 호출하면 appMain에 복붙된 프레임 그리기 코드가 한 곳으로 모입니다.

### 3단계 — 드라이버 패턴 통일

단일 보드 프로젝트이므로 **함수포인터 + `s_active_*` 싱글턴을 제거**하고 `xxxRead(handle*, ...)` 스타일로 통일하는 것을 권장합니다.

```c
// 현재
hdht11.read(&dht_data);            // 내부에서 s_active_dht 사용 → 인스턴스 1개만 가능
// 변경
dht11Read(&hdht11, &dht_data);     // 핸들 명시, RAM 절약, 다중 인스턴스 가능
```
정말 다형성이 필요한 곳(SSD1306 전송 계층)만 인터페이스 구조체를 두면 됩니다:
```c
typedef struct {
  void (*writeCmd)(const uint8_t *buf, uint16_t len);
  void (*writeData)(const uint8_t *buf, uint16_t len);
  void (*reset)(void);
} ssd1306Port_t;

extern const ssd1306Port_t ssd1306PortSpi;   // bit-bang 또는 HW SPI
```
그러면 `bsp_ssd1306.c`(I2C)는 삭제하거나 `ssd1306_port_i2c.c` 60줄 정도로 축소되고, 폰트·그래픽 코드 중복 ~250줄이 사라집니다.

### 4단계 — 성능 개선 (선택)

| 항목 | 현재 | 제안 |
|---|---|---|
| OLED 전송 | GPIO bit‑bang, ~25k HAL 호출/프레임 | SPI1 하드웨어 + DMA (`HAL_SPI_Transmit_DMA`), CubeMX에 SPI1 추가 |
| HC‑SR04 | 최대 35ms 블로킹 폴링 | Echo 핀 → TIM 입력 캡처(또는 EXTI 양엣지 + DWT) 로 비차단화 |
| DHT11 | `HAL_Delay(18)` + IRQ 5ms 차단 | 18ms LOW 구간을 스케줄러 2단계로 분리(시작 → 다음 틱에 읽기), DWT 타이밍으로 임계구역 최소화 |
| LCD1602 | 1초마다 32문자 전부 재전송 | 이전 문자열과 diff 후 변경 시만 전송 |
| OLED | 매 100ms 전체 1KB 전송 | 값 변경 시에만 `update()`, 또는 페이지 단위 dirty flag |

### 5단계 — 정리·품질

- 사용하지 않는 코드 삭제: `timerSetDutyFloat`, `s_current_duty`, `is_n_channel`, `bgr*` GPIO 제어 함수, `SPI1_SS_Pin`, `i2cInit()`, `ssd1306SpiTest`, `SSD1306_SPI_COLOR_YELLOW/BLUE`.
- `bsp_timer.c` → `bsp_pwm_led.c`로 이름 변경 및 TIM8 관련 주석 전면 수정.
- 모든 핀 초기화를 CubeMX(`gpio.c`)로 이관하고 `.ioc`에 Label 등록 → `main.h`의 USER CODE 핀 정의를 CubeMX 생성 코드로 대체. 드라이버는 `HAL_GPIO_Init`을 호출하지 않고 핀 정보만 받도록.
- `cli.csolution.yml` `misc` 에 `-Wall -Wextra -Wshadow -Wdouble-promotion` 추가 (`float` 연산에서 `double` 승격 감지에 유용).
- `main.c`의 `APBPrescTable` 정의에 이유 주석 추가 또는 `Core/Src/system_stm32f1xx.c` 사용으로 정리.
- `DOC/Phase1_Software_Architecture.md`는 잘 작성되어 있으므로, 리팩토링 후 모듈 이름과 파일 경로를 문서에 반영.

---

## 우선순위 요약

| 순위 | 작업 | 효과 |
|---|---|---|
| 1 | `hcSr04Init` NULL 버그, DWT 기반 `bsp_delay` 통합 | Release 빌드 정상 동작 |
| 2 | TIM2 Period 통일, BGR_GND 출력 설정, 죽은 EXTI 콜백 제거 | LED 플리커 제거, 하드웨어 안전 |
| 3 | `appMain` → scheduler / sensors / power / ui / log 분리 | 가독성·테스트 용이성 |
| 4 | SSD1306 gfx/port 분리, 폰트 단일화, I2C 버전 정리 | 중복 ~250줄 제거 |
| 5 | 드라이버 패턴 통일 (`s_active_*` 제거) | RAM 절약, 일관성 |
| 6 | HW SPI/DMA, 입력 캡처, dirty‑update | CPU 여유 확보 |

원하시면 1단계(버그 수정 + `bsp_delay` 공용화)부터 실제 코드 수정을 진행해 드리겠습니다. 어느 단계부터 시작할지 알려주세요.