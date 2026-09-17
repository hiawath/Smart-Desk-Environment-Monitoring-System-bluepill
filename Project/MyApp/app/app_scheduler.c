#include "app_scheduler.h"
#include "main.h"

void schedulerRun(task_t *tasks, size_t count)
{
  if (!tasks || count == 0)
    return;

  uint32_t now = HAL_GetTick();

  for (size_t i = 0; i < count; i++)
  {
    if (tasks[i].fn && (now - tasks[i].last_tick >= tasks[i].period_ms))
    {
      tasks[i].last_tick = now;
      tasks[i].fn();
    }
  }
}
