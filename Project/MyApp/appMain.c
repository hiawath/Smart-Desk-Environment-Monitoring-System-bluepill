
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
  // ssd1306Clear();
  // ssd1306DrawRect(0, 0, SSD1306_WIDTH, SSD1306_HEIGHT, SSD1306_COLOR_WHITE);
  // ssd1306DrawString(8, 3, "HC-SR04 RADAR", SSD1306_COLOR_WHITE);
  // ssd1306DrawLine(4, 13, 124, 13, SSD1306_COLOR_WHITE);
  // ssd1306DrawString(6, 20, "Distance:  ---.- cm", SSD1306_COLOR_WHITE);
  // ssd1306DrawRect(6, 32, 116, 10, SSD1306_COLOR_WHITE); /* 거리 바 게이지 테두리 */
  // ssd1306DrawString(6, 46, "Range : 2 - 200 cm", SSD1306_COLOR_WHITE);
  // ssd1306DrawString(6, 54, "Rate  : 100ms (10Hz)", SSD1306_COLOR_WHITE);
  // ssd1306Update();

  /* DMA 방식 ADC 내부 온도 측정 시작 (샘플링 주기: 500ms) */
  adcStartDMA(500);

  /* LCD1602 초기 화면 출력 */
  // hlcd1602.clear();
  // hlcd1602.cursor(0, 0);
  // hlcd1602.print("In: --.-C T:--.-C");
  // hlcd1602.cursor(1, 0);
  // hlcd1602.print("Humidity:  --.-%");

  uint32_t prev_100ms_tick    = HAL_GetTick();
  uint32_t prev_250ms_tick    = HAL_GetTick();
  uint32_t prev_500ms_tick    = HAL_GetTick();
  uint32_t prev_1s_tick    = HAL_GetTick();
  uint32_t prev_2s_tick    = HAL_GetTick() + 500; /* DHT11은 500ms 오프셋으로 분리하여 I2C 경합 방지 */
  char str_buf[32];

  ds1302Time_t rtc_time = {0};
  dht11Data_t dht_data  = {0};
  float distance_cm     = 0.0f;

  /* 첫 1회 초기값 읽기 */
  // ds1302GetDateTime(&hds1302, &rtc_time);
  // hdht11.read(&dht_data);

  uint32_t frame_count      = 0;
  
  float raw_dist = 0.0f;
  float smooth_dist = 0.0f;
  static const char spinner[] = {'|', '/', '-', '\\'};

  while (1)
  {

    timerLedUpdate();
    /* ADC 샘플링 주기 관리 및 DMA 완료 시 내부 온도 계산 */
    adcUpdate();

    /* 1. 2000ms(2초)마다 DHT11 온습도 센서 읽기 (1s 작업과 오프셋 분리) */
    if (HAL_GetTick() - prev_2s_tick >= 2000)
    {
      prev_2s_tick = HAL_GetTick();
    }

    /* 2. 1000ms(1초)마다 RTC 시간, CLCD, UART 로그 갱신 */
    if (HAL_GetTick() - prev_1s_tick >= 1000)
    {
      prev_1s_tick = HAL_GetTick();

      /* DS1302 RTC 읽기 */
      // ds1302GetDateTime(&hds1302, &rtc_time);
      //       /* ─── UART 시리얼 로그 출력 ─── */
      // printf("[DATA] %04d-%02d-%02d (%s) %02d:%02d:%02d | DHT11: %.1f C, %.1f %% | Dist: %.1f cm \r\n",
      //        rtc_time.year, rtc_time.month, rtc_time.day, ds1302GetDayStr(rtc_time.day_of_week),
      //        rtc_time.hour, rtc_time.min, rtc_time.sec,
      //        dht_data.temperature, dht_data.humidity, distance_cm);

     

    }

    if (HAL_GetTick() - prev_500ms_tick >= 500)
    {
      prev_500ms_tick = HAL_GetTick();
      /* ─── LCD1602(CLCD): 내부온도 / DHT11 온습도 갱신 ─── */
            /* 내부 온도 취득 */
      float int_temp = adcGetTemp();
      printf("int Temp : %f\r\n", int_temp);
      ld2Toggle();
      // hdht11.read(&dht_data);
      // hlcd1602.cursor(0, 0);
      // hlcd1602.printf("In:%4.1fC T:%4.1fC", int_temp, dht_data.temperature);

      // hlcd1602.cursor(1, 0);
      // hlcd1602.printf("Humidity:  %4.1f%%", dht_data.humidity);

      

    }

    if (HAL_GetTick() - prev_250ms_tick >= 250)
    {
      prev_250ms_tick = HAL_GetTick();

    }


    /* 100ms(10Hz) 주기로 고속 측정 및 화면 부드럽게 갱신 */
    if (HAL_GetTick() - prev_100ms_tick >= 100)
    {
      prev_100ms_tick = HAL_GetTick();
      frame_count++;

      /* 초음파 거리 측정 */
      // bool dist_ok = hhcSr04.read(&raw_dist);

      /* 1. 상단 타이틀 우측 동작 스피너(Spinner) */
      // ssd1306FillRect(96, 3, 24, 8, SSD1306_COLOR_BLACK);
      // snprintf(str_buf, sizeof(str_buf), "[%c]", spinner[frame_count % 4]);
      // ssd1306DrawString(96, 3, str_buf, SSD1306_COLOR_WHITE);

      /* 2. 지수 이동 평균(EMA) 필터 적용으로 부드러운 거리 수치 계산 */
      // if (dist_ok && raw_dist >= 2.0f && raw_dist <= 200.0f)
      // {
      //   if (smooth_dist < 1.0f)
      //   {
      //     smooth_dist = raw_dist;
      //   }
      //   else
      //   {
      //     smooth_dist = (0.7f * raw_dist) + (0.3f * smooth_dist);
      //   }
      // }
      // else if (raw_dist == 0.0f)
      // {
      //   smooth_dist = 0.0f; /* 4회 이상 연속 실패 시만 0 표시 */
      // }
      // distance_cm=(float)smooth_dist;

      // /* 3. 거리 수치 텍스트 표시 (y=20) */
      // ssd1306FillRect(6, 20, 116, 9, SSD1306_COLOR_BLACK);
      // if (smooth_dist >= 2.0f)
      // {
      //   snprintf(str_buf, sizeof(str_buf), "Distance: %5.1f cm", smooth_dist);
      // }
      // else
      // {
      //   snprintf(str_buf, sizeof(str_buf), "Distance:  ---.- cm");
      // }
      // ssd1306DrawString(6, 20, str_buf, SSD1306_COLOR_WHITE);

      // /* 4. 거리 시각화 바 게이지 (y=32, 높이 10, 내부영역: 8, 34, 112, 6) */
      // ssd1306FillRect(8, 34, 112, 6, SSD1306_COLOR_BLACK);
      // if (smooth_dist >= 2.0f)
      // {
      //   /* 0 ~ 100cm 기준으로 게이지 채우기 */
      //   int16_t bar_len = (int16_t)((smooth_dist / 100.0f) * 112.0f);
      //   if (bar_len > 112) bar_len = 112;
      //   if (bar_len < 0) bar_len = 0;
      //   if (bar_len > 0)
      //   {
      //     ssd1306FillRect(8, 34, bar_len, 6, SSD1306_COLOR_WHITE);
      //   }
      // }

      // /* 5. OLED 화면 전송 (400kHz I2C 약 25ms 소요) */
      // ssd1306Update();
    }

  }
}
