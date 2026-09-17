#include "ui_lcd.h"
#include "app_data.h"
#include "appInit.h"
#include <stdio.h>
#include <string.h>

static char s_prev_line0[17] = "";
static char s_prev_line1[17] = "";

void uiLcdInit(void)
{
  lcd1602Clear(&hlcd1602);
  lcd1602Cursor(&hlcd1602, 0, 0);
  lcd1602Print(&hlcd1602, "Smart Desk Sys");
  lcd1602Cursor(&hlcd1602, 1, 0);
  lcd1602Print(&hlcd1602, "Initializing...");
  memset(s_prev_line0, 0, sizeof(s_prev_line0));
  memset(s_prev_line1, 0, sizeof(s_prev_line1));
}

void uiLcdSleep(void)
{
  lcd1602Clear(&hlcd1602);
  lcd1602Backlight(&hlcd1602, false);
  memset(s_prev_line0, 0, sizeof(s_prev_line0));
  memset(s_prev_line1, 0, sizeof(s_prev_line1));
}

void uiLcdWake(void)
{
  lcd1602Backlight(&hlcd1602, true);
  memset(s_prev_line0, 0, sizeof(s_prev_line0));
  memset(s_prev_line1, 0, sizeof(s_prev_line1));
}

void uiLcdRender(void)
{
  if (!g_sys.screen_on)
    return;

  char cur_line0[24];
  char cur_line1[24];

  snprintf(cur_line0, sizeof(cur_line0), "%02d:%02d:%02d  %4.1fC",
           g_sys.rtc_time.hour, g_sys.rtc_time.min, g_sys.rtc_time.sec,
           g_sys.temperature_c);

  snprintf(cur_line1, sizeof(cur_line1), "H:%2.0f%% D:%3.0f M:%.0fC",
           g_sys.humidity_pct, g_sys.distance_cm, g_sys.mcu_temp_c);

  /* 라인 0 내용이 변경되었을 때만 I2C 전송 (16문자 정밀 비교) */
  if (strncmp(cur_line0, s_prev_line0, 16) != 0)
  {
    lcd1602Cursor(&hlcd1602, 0, 0);
    lcd1602Print(&hlcd1602, cur_line0);
    strncpy(s_prev_line0, cur_line0, 16);
    s_prev_line0[16] = '\0';
  }

  /* 라인 1 내용이 변경되었을 때만 I2C 전송 (16문자 정밀 비교) */
  if (strncmp(cur_line1, s_prev_line1, 16) != 0)
  {
    lcd1602Cursor(&hlcd1602, 1, 0);
    lcd1602Print(&hlcd1602, cur_line1);
    strncpy(s_prev_line1, cur_line1, 16);
    s_prev_line1[16] = '\0';
  }
}
