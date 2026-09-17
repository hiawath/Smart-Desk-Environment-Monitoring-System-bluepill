#include "app_log.h"
#include "app_data.h"
#include <stdio.h>

void logOutput(void)
{
  printf("[%02d:%02d:%02d] T:%.1fC, H:%.1f%%, D:%.1fcm, McuT:%.1fC [Screen:%s]\r\n",
         g_sys.rtc_time.hour, g_sys.rtc_time.min, g_sys.rtc_time.sec,
         g_sys.temperature_c, g_sys.humidity_pct, g_sys.distance_cm, g_sys.mcu_temp_c,
         g_sys.screen_on ? "ON" : "OFF");
}
