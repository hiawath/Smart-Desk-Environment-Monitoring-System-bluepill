#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
  uint32_t period_ms;
  uint32_t last_tick;
  void (*fn)(void);
} task_t;

void schedulerRun(task_t *tasks, size_t count);
