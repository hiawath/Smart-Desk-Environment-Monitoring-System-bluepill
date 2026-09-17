#pragma once

#include <stdint.h>
#include <stdbool.h>

/* --- 스케줄러 태스크 실행 주기 (ms) --- */
#define TASK_FAST_MS         100U   /* 초음파 거리 측정, EMA, 스마트 파워, OLED 갱신 */
#define TASK_MID_MS          500U   /* 내부 MCU 온도 갱신, 상태 LED 토글 */
#define TASK_SLOW_MS         1000U  /* RTC 시간 갱신, LCD1602 갱신, UART 로그 */
#define TASK_ENV_MS          2000U  /* DHT11 온습도 측정 (오프셋 500ms 권장) */

/* --- 스마트 파워 (절전 모드) 파라미터 --- */
#define SLEEP_TIMEOUT_MS     10000U /* 10초 이상 미감지 시 자동 절전 */
#define PRESENCE_DIST_MAX    60.0f  /* 착석 감지 최대 거리 (cm) */
#define PRESENCE_DIST_MIN    2.0f   /* 착석 감지 최소 거리 (cm) */

/* --- 필터 파라미터 --- */
#define EMA_ALPHA_HCSR04     0.7f   /* 초음파 거리 지수이동평균 계수 (0.7 신규 + 0.3 이전) */
