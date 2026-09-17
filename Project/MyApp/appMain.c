
#include "bsp/bsp_gpio.h"
#include "bsp/bsp_timer.h"
#include "main.h"
#include "appMain.h"
#include "appInit.h"
#include <stdio.h>

/**
  * @brief  Application Main Entry Point (메인 루프)
  */
void appMain(void)
{
  /* SSD1306 초기 화면 프레임 및 타이틀 출력 */
  hssd1306Spi.clear();
  hssd1306Spi.drawRect(0, 0, SSD1306_SPI_WIDTH, SSD1306_SPI_HEIGHT, SSD1306_SPI_COLOR_WHITE);
  hssd1306Spi.drawString(8, 3, "SMART DESK MON", SSD1306_SPI_COLOR_WHITE);
  hssd1306Spi.drawLine(4, 13, 124, 13, SSD1306_SPI_COLOR_WHITE);
  hssd1306Spi.update();

  /* DMA 방식 ADC 내부 온도 측정 시작 (샘플링 주기: 500ms) */
  adcStartDMA(500);

  /* LCD1602 초기 화면 출력 */
  hlcd1602.clear();
  hlcd1602.cursor(0, 0);
  hlcd1602.print("Smart Desk Sys");
  hlcd1602.cursor(1, 0);
  hlcd1602.print("Initializing...");

  uint32_t prev_100ms_tick    = HAL_GetTick();
  uint32_t prev_500ms_tick    = HAL_GetTick();
  uint32_t prev_1s_tick    = HAL_GetTick();
  uint32_t prev_2s_tick    = HAL_GetTick() + 500; 
  char str_buf[32];

  ds1302Time_t rtc_time = {0};
  dht11Data_t dht_data  = {0};
  float distance_cm     = 0.0f;
  float int_temp        = 0.0f;

  /* 첫 1회 초기값 읽기 */
  ds1302GetDateTime(&hds1302, &rtc_time);
  hdht11.read(&dht_data);

  uint32_t frame_count      = 0;
  float smooth_dist = 0.0f;
  static const char spinner[] = {'|', '/', '-', '\\'};

#define SLEEP_TIMEOUT_MS  10000  /* 10초 이상 미감지 시 자동 절전 */
#define PRESENCE_DIST_MAX 60.0f  /* 착석 감지 최대 거리 (cm) */
#define PRESENCE_DIST_MIN 2.0f   /* 착석 감지 최소 거리 (cm) */

  bool screen_on = true;
  uint32_t last_presence_tick = HAL_GetTick();

  while (1)
  {
    timerLedUpdate();
    adcUpdate();

    /* 1. 2000ms(2초)마다 DHT11 온습도 센서 읽기 */
    if (HAL_GetTick() - prev_2s_tick >= 2000)
    {
      prev_2s_tick = HAL_GetTick();
      hdht11.read(&dht_data);
    }

    /* 2. 1000ms(1초)마다 RTC 시간 갱신, LCD1602 및 UART 로그 출력 */
    if (HAL_GetTick() - prev_1s_tick >= 1000)
    {
      prev_1s_tick = HAL_GetTick();
      ds1302GetDateTime(&hds1302, &rtc_time);

      if (screen_on)
      {
        hlcd1602.cursor(0, 0);
        hlcd1602.printf("%02d:%02d:%02d  %4.1fC", rtc_time.hour, rtc_time.min, rtc_time.sec, dht_data.temperature);
        hlcd1602.cursor(1, 0);
        hlcd1602.printf("H:%2.0f%% D:%3.0f M:%2.0fC", dht_data.humidity, distance_cm, int_temp);
      }

      /* UART 원격 텔레메트리 로그 */
      printf("[%02d:%02d:%02d] T:%.1fC, H:%.1f%%, D:%.1fcm, McuT:%.1fC [Screen:%s]\r\n", 
             rtc_time.hour, rtc_time.min, rtc_time.sec, 
             dht_data.temperature, dht_data.humidity, distance_cm, int_temp,
             screen_on ? "ON" : "OFF");
    }

    /* 3. 500ms마다 내부 온도 및 상태 LED 토글 */
    if (HAL_GetTick() - prev_500ms_tick >= 500)
    {
      prev_500ms_tick = HAL_GetTick();
      int_temp = adcGetTemp();
      ld2Toggle();
    }

    /* 4. 100ms 주기로 초음파 거리 측정, EMA 필터링, 스마트 파워 및 OLED 갱신 */
    if (HAL_GetTick() - prev_100ms_tick >= 100)
    {
      prev_100ms_tick = HAL_GetTick();
      frame_count++;

      float raw_dist = 0.0f;
      bool dist_ok = hhcSr04.read(&raw_dist);

      if (dist_ok && raw_dist > 0)
      {
        smooth_dist = (0.7f * raw_dist) + (0.3f * smooth_dist);
        distance_cm = smooth_dist;
      }

      /* --- 스마트 파워 (Smart Screen) 상태 머신 --- */
      if (distance_cm >= PRESENCE_DIST_MIN && distance_cm <= PRESENCE_DIST_MAX)
      {
        /* 사용자 근접 감지 -> 착석 타이머 갱신 */
        last_presence_tick = HAL_GetTick();

        if (!screen_on)
        {
          /* 절전 모드에서 기상(Wake-up) */
          screen_on = true;
          hlcd1602.backlight(true);

          /* OLED 기본 프레임 및 타이틀 복구 */
          hssd1306Spi.clear();
          hssd1306Spi.drawRect(0, 0, SSD1306_SPI_WIDTH, SSD1306_SPI_HEIGHT, SSD1306_SPI_COLOR_WHITE);
          hssd1306Spi.drawString(8, 3, "SMART DESK MON", SSD1306_SPI_COLOR_WHITE);
          hssd1306Spi.drawLine(4, 13, 124, 13, SSD1306_SPI_COLOR_WHITE);
          hssd1306Spi.update();
          printf("[POWER] User detected (%.1fcm) -> Display ON\r\n", distance_cm);
        }
      }
      else
      {
        /* 일정 시간 동안 사용자 미감지 시 자동 절전 */
        if (screen_on && (HAL_GetTick() - last_presence_tick >= SLEEP_TIMEOUT_MS))
        {
          screen_on = false;
          hlcd1602.clear();
          hlcd1602.backlight(false);

          hssd1306Spi.clear();
          hssd1306Spi.update();
          printf("[POWER] User absent for %d sec -> Display OFF (Sleep)\r\n", SLEEP_TIMEOUT_MS / 1000);
        }
      }

      /* --- OLED UI 렌더링 (화면이 켜져 있을 때만 동작) --- */
      if (screen_on)
      {
        /* 동적 갱신 영역 지우기 (Y: 15 ~ 62) */
        hssd1306Spi.fillRect(4, 15, 120, 47, SSD1306_SPI_COLOR_BLACK);

        /* 1. 실시간 시계 (중앙 정렬) */
        snprintf(str_buf, sizeof(str_buf), "%02d:%02d:%02d", rtc_time.hour, rtc_time.min, rtc_time.sec);
        hssd1306Spi.drawString(38, 17, str_buf, SSD1306_SPI_COLOR_WHITE);

        /* 2. 환경 센싱 정보 (온습도 및 내부온도) */
        snprintf(str_buf, sizeof(str_buf), "T:%4.1fC H:%4.1f%%", dht_data.temperature, dht_data.humidity);
        hssd1306Spi.drawString(8, 29, str_buf, SSD1306_SPI_COLOR_WHITE);

        snprintf(str_buf, sizeof(str_buf), "MCU Temp: %4.1fC", int_temp);
        hssd1306Spi.drawString(8, 40, str_buf, SSD1306_SPI_COLOR_WHITE);

        /* 3. 근접 거리 프로그래스바 게이지 (0 ~ 100cm 기준) */
        hssd1306Spi.drawRect(8, 52, 112, 8, SSD1306_SPI_COLOR_WHITE);
        int bar_w = (int)((distance_cm / 100.0f) * 108.0f);
        if (bar_w > 108) bar_w = 108;
        if (bar_w > 0)
        {
          hssd1306Spi.fillRect(10, 54, bar_w, 4, SSD1306_SPI_COLOR_WHITE);
        }

        /* 4. 활동 상태 스피너 (Y=3 우측 끝) */
        hssd1306Spi.drawChar(115, 3, spinner[frame_count % 4], SSD1306_SPI_COLOR_WHITE);

        hssd1306Spi.update();
      }
    }
  }
}
