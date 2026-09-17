
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

  bool screen_on = true;

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

    /* 2. 1000ms(1초)마다 RTC 시간 및 CLCD 상단 정보 갱신 */
    if (HAL_GetTick() - prev_1s_tick >= 1000)
    {
      prev_1s_tick = HAL_GetTick();
      ds1302GetDateTime(&hds1302, &rtc_time);

      if (screen_on) {
        hlcd1602.cursor(0, 0);
        hlcd1602.printf("%02d:%02d:%02d  %4.1fC", rtc_time.hour, rtc_time.min, rtc_time.sec, dht_data.temperature);
        hlcd1602.cursor(1, 0);
        hlcd1602.printf("H:%4.1f%% D:%5.1f", dht_data.humidity, distance_cm);
      }

      /* UART 로그 */
      printf("[%02d:%02d:%02d] T:%.1f, H:%.1f, D:%.1f\r\n", 
             rtc_time.hour, rtc_time.min, rtc_time.sec, 
             dht_data.temperature, dht_data.humidity, distance_cm);
    }

    /* 3. 500ms마다 내부 온도 및 LED 상태 갱신 */
    if (HAL_GetTick() - prev_500ms_tick >= 500)
    {
      prev_500ms_tick = HAL_GetTick();
      int_temp = adcGetTemp();
      ld2Toggle();
    }

    /* 4. 100ms 주기로 고속 측정 (초음파) 및 OLED 갱신 */
    if (HAL_GetTick() - prev_100ms_tick >= 100)
    {
      prev_100ms_tick = HAL_GetTick();
      frame_count++;

      float raw_dist = 0.0f;
      bool dist_ok = hhcSr04.read(&raw_dist);

      if (dist_ok && raw_dist > 0) {
        smooth_dist = (0.7f * raw_dist) + (0.3f * smooth_dist);
        distance_cm = smooth_dist;
      }

      /* 초음파 기반 자동 화면 절전 기능 (Smart Screen) */
      /* 50cm 이내에 물체가 있으면 화면 켬, 없으면 끔 */
      if (distance_cm < 50.0f && distance_cm > 2.0f) {
        if (!screen_on) {
          screen_on = true;
          hlcd1602.backlight(true);
        }
      } else if (distance_cm > 100.0f || distance_cm == 0.0f) {
        // if (screen_on) {
        //   screen_on = false;
        //   hlcd1602.backlight(false);
        //   hssd1306Spi.clear();
        //   hssd1306Spi.update();
        // }
      }

      if (screen_on) {
        /* OLED UI 갱신 */
        hssd1306Spi.fillRect(6, 20, 116, 40, SSD1306_SPI_COLOR_BLACK);
        
        /* 시간 표시 */
        snprintf(str_buf, sizeof(str_buf), "%02d:%02d:%02d", rtc_time.hour, rtc_time.min, rtc_time.sec);
        hssd1306Spi.drawString(35, 20, str_buf, SSD1306_SPI_COLOR_WHITE);

        /* 온습도 표시 */
        snprintf(str_buf, sizeof(str_buf), "T:%.1fC H:%.1f%%", dht_data.temperature, dht_data.humidity);
        hssd1306Spi.drawString(10, 35, str_buf, SSD1306_SPI_COLOR_WHITE);

        /* 거리 게이지 */
        hssd1306Spi.drawRect(10, 50, 108, 8, SSD1306_SPI_COLOR_WHITE);
        int bar_w = (int)((distance_cm / 100.0f) * 106.0f);
        if (bar_w > 106) bar_w = 106;
        if (bar_w > 0) hssd1306Spi.fillRect(11, 51, bar_w, 6, SSD1306_SPI_COLOR_WHITE);

        /* 스피너 */
        hssd1306Spi.drawChar(115, 3, spinner[frame_count % 4], SSD1306_SPI_COLOR_WHITE);
        
        hssd1306Spi.update();
      }
    }
  }
}
