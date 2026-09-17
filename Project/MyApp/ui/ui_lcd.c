#include "ui_lcd.h"
#include "app_data.h"
#include "appInit.h"

void uiLcdInit(void)
{
  hlcd1602.clear();
  hlcd1602.cursor(0, 0);
  hlcd1602.print("Smart Desk Sys");
  hlcd1602.cursor(1, 0);
  hlcd1602.print("Initializing...");
}

void uiLcdSleep(void)
{
  hlcd1602.clear();
  hlcd1602.backlight(false);
}

void uiLcdWake(void)
{
  hlcd1602.backlight(true);
}

void uiLcdRender(void)
{
  if (!g_sys.screen_on)
    return;

  hlcd1602.cursor(0, 0);
  hlcd1602.printf("%02d:%02d:%02d  %4.1fC",
                  g_sys.rtc_time.hour, g_sys.rtc_time.min, g_sys.rtc_time.sec,
                  g_sys.temperature_c);
  hlcd1602.cursor(1, 0);
  hlcd1602.printf("H:%2.0f%% D:%3.0f M:%2.0fC",
                  g_sys.humidity_pct, g_sys.distance_cm, g_sys.mcu_temp_c);
}
