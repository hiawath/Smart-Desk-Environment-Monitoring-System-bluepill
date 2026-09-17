#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "bsp_ds1302.h"

typedef struct {
  ds1302Time_t rtc_time;
  float temperature_c;     /* DHT11 온도 (°C) */
  float humidity_pct;      /* DHT11 습도 (%) */
  float distance_cm;       /* EMA 적용된 유효 거리 (cm) */
  float raw_distance_cm;   /* 초음파 원시 측정 거리 (cm) */
  float mcu_temp_c;        /* MCU 내부 온도 센서 (°C) */
  bool  dht_valid;
  bool  dist_valid;
  bool  screen_on;         /* 화면 ON/OFF 상태 */
  uint32_t frame_count;    /* OLED 렌더링 프레임 카운트 */
} sysData_t;

extern sysData_t g_sys;

void appDataInit(void);
