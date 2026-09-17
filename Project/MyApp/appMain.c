#include "appMain.h"
#include "appInit.h"
#include "main.h"

#include "app/app_config.h"
#include "app/app_data.h"
#include "app/app_scheduler.h"
#include "app/app_sensors.h"
#include "app/app_power.h"
#include "app/app_log.h"
#include "ui/ui_oled.h"
#include "ui/ui_lcd.h"
#include "bsp/bsp_timer.h"
#include "bsp/bsp_adc.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* 100ms: 초음파 거리 측정 -> EMA 필터 -> 파워 FSM -> OLED 렌더링 */
static void taskFast(void)
{
  sensorsReadDistance();
  powerUpdate();
  uiOledRender();
}

/* 500ms: MCU 내부 온도 읽기 및 상태 LED 토글 */
static void taskMid(void)
{
  sensorsReadMcuTemp();
}

/* 1000ms: RTC 시각 갱신 -> LCD1602 렌더링 -> UART 로그 출력 */
static void taskSlow(void)
{
  sensorsReadRtc();
  uiLcdRender();
  logOutput();
}

/* 2000ms: DHT11 온습도 측정 (타 태스크와 분산 위해 500ms 오프셋) */
static void taskEnv(void)
{
  sensorsReadEnv();
}

static task_t s_tasks[] = {
  { TASK_FAST_MS,   0,   taskFast },
  { TASK_MID_MS,    0,   taskMid  },
  { TASK_SLOW_MS,   0,   taskSlow },
  { TASK_ENV_MS,  500U,  taskEnv  },
};

void appMain(void)
{
  /* 1. 데이터 모델 및 UI 초기화 */
  appDataInit();
  uiOledInit();
  uiLcdInit();
  powerInit();
  sensorsInit();

  /* 2. 백그라운드 드라이버 시작 */
  adcStartDMA(TASK_MID_MS);

  /* 3. 비차단 협력형 스케줄러 메인 루프 */
  while (1)
  {
    timerLedUpdate();
    adcUpdate();
    schedulerRun(s_tasks, ARRAY_SIZE(s_tasks));
  }
}
