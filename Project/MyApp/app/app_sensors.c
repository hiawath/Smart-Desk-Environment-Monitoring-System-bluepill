#include "app_sensors.h"
#include "app_data.h"
#include "app_config.h"
#include "appInit.h"
#include "bsp_gpio.h"

void sensorsInit(void)
{
  /* 부팅 시 1회 초기 센서 스냅샷 읽기 */
  ds1302GetDateTime(&hds1302, &g_sys.rtc_time);

  dht11Data_t dht_data = {0};
  if (hdht11.read(&dht_data))
  {
    g_sys.temperature_c = dht_data.temperature;
    g_sys.humidity_pct  = dht_data.humidity;
    g_sys.dht_valid     = true;
  }
}

void sensorsReadEnv(void)
{
  dht11Data_t dht_data = {0};
  if (hdht11.read(&dht_data))
  {
    g_sys.temperature_c = dht_data.temperature;
    g_sys.humidity_pct  = dht_data.humidity;
    g_sys.dht_valid     = true;
  }
}

void sensorsReadMcuTemp(void)
{
  g_sys.mcu_temp_c = adcGetTemp();
  ld2Toggle(); /* 상태 LED 토글 (500ms 주기) */
}

void sensorsReadRtc(void)
{
  ds1302GetDateTime(&hds1302, &g_sys.rtc_time);
}

void sensorsReadDistance(void)
{
  float raw_dist = 0.0f;
  bool ok = hhcSr04.read(&raw_dist);

  if (ok && raw_dist > 0.0f)
  {
    g_sys.raw_distance_cm = raw_dist;
    if (!g_sys.dist_valid || g_sys.distance_cm <= 0.0f)
    {
      g_sys.distance_cm = raw_dist;
    }
    else
    {
      /* EMA 필터 적용: new * alpha + old * (1 - alpha) */
      g_sys.distance_cm = (EMA_ALPHA_HCSR04 * raw_dist) + ((1.0f - EMA_ALPHA_HCSR04) * g_sys.distance_cm);
    }
    g_sys.dist_valid = true;
  }
}
