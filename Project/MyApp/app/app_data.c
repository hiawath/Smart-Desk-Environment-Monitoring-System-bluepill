#include "app_data.h"
#include <string.h>

sysData_t g_sys;

void appDataInit(void)
{
  memset(&g_sys, 0, sizeof(sysData_t));
  g_sys.screen_on = true;
}
