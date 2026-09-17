#include "app_power.h"
#include "app_data.h"
#include "app_config.h"
#include "ui/ui_oled.h"
#include "ui/ui_lcd.h"
#include "main.h"
#include <stdio.h>

static uint32_t s_last_presence_tick = 0;

void powerInit(void)
{
  s_last_presence_tick = HAL_GetTick();
  g_sys.screen_on = true;
}

void powerUpdate(void)
{
  uint32_t now = HAL_GetTick();
  bool user_present = (g_sys.distance_cm >= PRESENCE_DIST_MIN &&
                       g_sys.distance_cm <= PRESENCE_DIST_MAX);

  if (user_present)
  {
    s_last_presence_tick = now;
    if (!g_sys.screen_on)
    {
      /* 절전 모드에서 기상 (Wake-up) */
      g_sys.screen_on = true;
      uiLcdWake();
      uiOledDrawFrame();
      printf("[POWER] User detected (%.1fcm) -> Display ON\r\n", g_sys.distance_cm);
    }
  }
  else
  {
    /* 일정 시간 동안 사용자 미감지 시 자동 절전 (Sleep) */
    if (g_sys.screen_on && (now - s_last_presence_tick >= SLEEP_TIMEOUT_MS))
    {
      g_sys.screen_on = false;
      uiLcdSleep();
      uiOledSleep();
      printf("[POWER] User absent for %u sec -> Display OFF (Sleep)\r\n",
             (unsigned int)(SLEEP_TIMEOUT_MS / 1000U));
    }
  }
}
