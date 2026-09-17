#include "ui_lcd.h"
#include "app_data.h"
#include "appInit.h"

void uiLcdInit(void)
{
  lcd1602Clear(&hlcd1602);
  lcd1602Cursor(&hlcd1602, 0, 0);
  lcd1602Print(&hlcd1602, "Smart Desk Sys");
  lcd1602Cursor(&hlcd1602, 1, 0);
  lcd1602Print(&hlcd1602, "Initializing...");
}

void uiLcdSleep(void)
{
  lcd1602Clear(&hlcd1602);
  lcd1602Backlight(&hlcd1602, false);
}

void uiLcdWake(void)
{
  lcd1602Backlight(&hlcd1602, true);
}

void uiLcdRender(void)
{
  if (!g_sys.screen_on)
    return;

  lcd1602Cursor(&hlcd1602, 0, 0);
  lcd1602Printf(&hlcd1602, "%02d:%02d:%02d  %4.1fC",
                g_sys.rtc_time.hour, g_sys.rtc_time.min, g_sys.rtc_time.sec,
                g_sys.temperature_c);
  lcd1602Cursor(&hlcd1602, 1, 0);
  lcd1602Printf(&hlcd1602, "H:%2.0f%% D:%3.0f M:%2.0fC",
                g_sys.humidity_pct, g_sys.distance_cm, g_sys.mcu_temp_c);
}
