#pragma once

void sensorsInit(void);
void sensorsReadEnv(void);      /* DHT11 온습도 갱신 (2000ms 주기) */
void sensorsReadMcuTemp(void);  /* 내부 MCU 온도 갱신 (500ms 주기) */
void sensorsReadRtc(void);      /* RTC 시각 갱신 (1000ms 주기) */
void sensorsReadDistance(void); /* 초음파 거리 측정 및 EMA 필터 (100ms 주기) */
