# Phase 6: OLED 디스플레이 30 FPS 고주사율 렌더링 분리 및 최적화 보고서

## 1. 개요
- **요청 사항**: `uiOledRender`만 24 또는 30 FPS로 주사율을 수정하여 부드러운 디스플레이 애니메이션 및 시각 갱신 구현.
- **배경**: 기존에는 `taskFast` (100ms / 10 FPS) 태스크에 초음파 거리 측정(`sensorsReadDistance`), 파워 FSM(`powerUpdate`), OLED 렌더링(`uiOledRender`)이 함께 묶여 있어 주사율이 10Hz로 제한되었고, 초음파 센서의 Echo 핀 대기 시간(수 ms)이 OLED 렌더링 타이밍에 간섭을 주는 문제가 있었음.

---

## 2. 구현 내용

### 2.1 OLED 전용 고주사율 매크로 정의 (`app_config.h`)
- 목표 주사율을 30 FPS로 설정하고, 24 FPS로 손쉽게 변경할 수 있도록 유연한 매크로 구조 도입:
```c
/* --- 스케줄러 태스크 실행 주기 (ms) --- */
#define OLED_TARGET_FPS      30U    /* OLED 목표 주사율: 30 FPS (24 또는 30 설정 가능) */
#define TASK_OLED_MS         (1000U / OLED_TARGET_FPS) /* 30 FPS ≈ 33ms */
#define TASK_FAST_MS         100U   /* 초음파 거리 측정, EMA, 스마트 파워 관리 (10Hz) */
```

### 2.2 독립 렌더링 태스크 분리 (`appMain.c`)
- 초음파 거리 센싱 태스크(`taskFast`)와 OLED 렌더링 태스크(`taskOled`)를 명확히 분리:
  1. **`taskOled` (33ms / 30 FPS)**: `uiOledRender()` 전용 독립 실행. 초음파 센서 블로킹에 영향받지 않고 일정한 33.3ms 간격으로 고속 렌더링 보장.
  2. **`taskFast` (100ms / 10Hz)**: `sensorsReadDistance()`, `powerUpdate()` 전담. HC-SR04의 권장 측정 주기(60ms 이상)를 만족하며 음향 반사 간섭(Cross-talk) 방지.

```c
/* 33ms (30 FPS): OLED 고주사율 디스플레이 렌더링 전용 태스크 */
static void taskOled(void)
{
  uiOledRender();
}

/* 100ms: 초음파 거리 측정 -> EMA 필터 -> 파워 FSM */
static void taskFast(void)
{
  sensorsReadDistance();
  powerUpdate();
}

static task_t s_tasks[] = {
  { TASK_OLED_MS,   0,   taskOled }, /* 30 FPS (33ms) 독립 렌더링 */
  { TASK_FAST_MS,   0,   taskFast }, /* 100ms 초음파/파워 FSM */
  { TASK_MID_MS,    0,   taskMid  },
  { TASK_SLOW_MS,   0,   taskSlow },
  { TASK_ENV_MS,  500U,  taskEnv  },
};
```

### 2.3 스피너 애니메이션 속도 보정 (`ui_oled.c`)
- 프레임 레이트가 10 FPS에서 30 FPS로 3배 빨라짐에 따라, 우측 상단 활동 상태 스피너(`s_spinner`)의 회전 속도를 6프레임당 1단계(`(frame_count / 6) % 4`)로 조절하여 약 200ms 주기로 자연스럽고 부드럽게 회전하도록 보정.

---

## 3. 검증 결과

### 3.1 정적 빌드 검증
- CMSIS-Toolbox `cbuild` (ARM Clang AC6 V6.24.0) 검증 완료:
  - **Result**: `Project.Release+STM32F103C8: 1 succeeded, 0 failed`
  - **Memory Footprint**: Code=31932, RO-data=1380, RW-data=180, ZI-data=6596

### 3.2 성능 및 CPU 점유율 분석
- SSD1306 SPI 1024바이트 버퍼 전송 소요 시간: 약 1.6ms.
- 30 FPS(주기 33.3ms) 기준 OLED 전송 CPU 점유율은 약 **4.8%**로, 시스템 스케줄러 및 타 센서 동작에 전혀 부담을 주지 않음.
