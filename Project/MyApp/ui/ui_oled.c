#include "ui_oled.h"
#include "app_data.h"
#include "bsp_ssd1306_spi.h"
#include <stdio.h>

static const char s_spinner[] = {'|', '/', '-', '\\'};

void uiOledDrawFrame(void)
{
  hssd1306Spi.clear();
  hssd1306Spi.drawRect(0, 0, SSD1306_SPI_WIDTH, SSD1306_SPI_HEIGHT, SSD1306_SPI_COLOR_WHITE);
  hssd1306Spi.drawString(8, 3, "SMART DESK MON", SSD1306_SPI_COLOR_WHITE);
  hssd1306Spi.drawLine(4, 13, 124, 13, SSD1306_SPI_COLOR_WHITE);
  hssd1306Spi.update();
}

void uiOledInit(void)
{
  uiOledDrawFrame();
}

void uiOledSleep(void)
{
  hssd1306Spi.clear();
  hssd1306Spi.update();
}

void uiOledRender(void)
{
  if (!g_sys.screen_on)
    return;

  char str_buf[32];

  /* 동적 영역 지우기 (Y: 15 ~ 62) */
  hssd1306Spi.fillRect(4, 15, 120, 47, SSD1306_SPI_COLOR_BLACK);

  /* 1. 실시간 시계 (중앙 정렬) */
  snprintf(str_buf, sizeof(str_buf), "%02d:%02d:%02d",
           g_sys.rtc_time.hour, g_sys.rtc_time.min, g_sys.rtc_time.sec);
  hssd1306Spi.drawString(38, 17, str_buf, SSD1306_SPI_COLOR_WHITE);

  /* 2. 환경 센싱 정보 (온습도 및 MCU 내부 온도) */
  snprintf(str_buf, sizeof(str_buf), "T:%4.1fC H:%4.1f%%",
           g_sys.temperature_c, g_sys.humidity_pct);
  hssd1306Spi.drawString(8, 29, str_buf, SSD1306_SPI_COLOR_WHITE);

  snprintf(str_buf, sizeof(str_buf), "MCU Temp: %4.1fC", g_sys.mcu_temp_c);
  hssd1306Spi.drawString(8, 40, str_buf, SSD1306_SPI_COLOR_WHITE);

  /* 3. 근접 거리 프로그레스 바 게이지 (0 ~ 100cm 기준) */
  hssd1306Spi.drawRect(8, 52, 112, 8, SSD1306_SPI_COLOR_WHITE);
  int bar_w = (int)((g_sys.distance_cm / 100.0f) * 108.0f);
  if (bar_w > 108) bar_w = 108;
  if (bar_w > 0)
  {
    hssd1306Spi.fillRect(10, 54, bar_w, 4, SSD1306_SPI_COLOR_WHITE);
  }

  /* 4. 활동 상태 스피너 (Y=3 우측 상단) */
  hssd1306Spi.drawChar(115, 3, s_spinner[g_sys.frame_count % 4], SSD1306_SPI_COLOR_WHITE);

  hssd1306Spi.update();
  g_sys.frame_count++;
}
