# Phase 2 리팩토링 보고서: `appMain` 모듈화 및 협력형 태스크 스케줄러 분리

## 1. 개요
기존 `appMain.c`는 단일 함수(~180줄) 내에 4개 주기 타이머, 5개 센서 데이터 수집, 스마트 파워 상태 머신, OLED 및 LCD1602 UI 렌더링, UART 로그 포맷팅, 로컬 매크로 정의까지 모두 혼재되어 있던 전형적인 'God Function' 구조였습니다.

Phase 2에서는 단일 책임 원칙(SRP)에 따라 기능별 모듈로 명확히 분리하고, 비차단 협력형 태스크 스케줄러(Cooperative Task Scheduler)를 도입하여 코드의 가독성과 유지보수성을 극대화하였습니다.

---

## 2. 모듈 분리 구조

```
Project/MyApp/
  app/
    app_config.h       # 주기(100ms, 500ms, 1s, 2s), 타임아웃, EMA 계수 등 시스템 상수
    app_data.c / h     # sysData_t 전역 데이터 모델 (센서 스냅샷, 화면 상태, 유효성 플래그)
    app_scheduler.c / h# 비차단 주기 태스크 테이블 디스패처 (schedulerRun)
    app_sensors.c / h  # 센서 수집 모듈 (DHT11, HC-SR04 EMA, RTC, MCU 온도)
    app_power.c / h    # 스마트 절전 FSM (Screen ON/OFF, 재실 감지, 기상 복구)
    app_log.c / h      # UART 텔레메트리 포맷팅 및 전송
  ui/
    ui_oled.c / h      # SSD1306 OLED 프레임, 시계, 센서값, 프로그레스바, 스피너 렌더링
    ui_lcd.c / h       # LCD1602 부팅 화면, 백라이트 제어, 2줄 레이아웃 렌더링
  appMain.c            # 태스크 등록 및 스케줄러 루프만 담당하는 순수 진입점 (~60줄)
```

---

## 3. 세부 개선 사항

### 3.1 협력형 태스크 스케줄러 (`app_scheduler.c/h`)
- `task_t` 구조체 배열로 주기 태스크들을 테이블 형태로 관리:
  ```c
  typedef struct {
    uint32_t period_ms;
    uint32_t last_tick;
    void (*fn)(void);
  } task_t;
  ```
- 메인 루프에서 `schedulerRun(s_tasks, count)`를 호출하여 `HAL_GetTick()` 차이를 기반으로 비차단 디스패치 수행.
- DHT11 측정(`taskEnv`)은 오프셋 500ms를 부여하여 타 태스크(거리 측정/OLED)와의 동시성 충돌 및 지터 방지.

### 3.2 스마트 파워 FSM 분리 (`app_power.c/h`)
- 기존에 부팅 시점과 Wake-up 시점에 완전히 중복되어 복붙되어 있던 OLED 프레임(`uiOledDrawFrame()`) 및 LCD 백라이트 복구 코드를 `powerUpdate()` 내부로 단일화.
- 사용자가 감지 범위(2~60cm)를 벗어나 10초 이상 경과 시 절전(Sleep) 모드로 전환.

### 3.3 UI 계층 분리 (`ui_oled.c/h`, `ui_lcd.c/h`)
- OLED의 동적 영역 지우기, 문자열 포맷팅, 프로그레스 바 계산, 프레임 카운터 스피너 로직을 `uiOledRender()`로 격리.
- LCD1602의 부팅 텍스트 출력, 백라이트 ON/OFF, 2줄 상태 출력을 `ui_lcd.c`로 격리.

### 3.4 데이터 모델 단일화 (`app_data.c/h`)
- 센서 값들과 화면 상태를 `sysData_t g_sys` 구조체로 중앙 관리하여, UI 및 통신 모듈이 센서 드라이버를 직접 호출하지 않고 캐시된 스냅샷 데이터를 안전하게 참조하도록 분리.

---

## 4. 검증 결과
- **컴파일러**: ARM Clang (AC6 V6.24.0), MicroLib
- **빌드 결과**: 1 succeeded, 0 failed (49개 타깃 정상 빌드)
- **프로그램 크기**: Code=38444, RO-data=1968, RW-data=216, ZI-data=6560
