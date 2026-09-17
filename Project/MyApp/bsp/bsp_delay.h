#pragma once

#include "main.h"
#include <stdint.h>

/**
 * @file  bsp_delay.h
 * @brief DWT(Data Watchpoint and Trace) 사이클 카운터 기반 마이크로초 딜레이/타임스탬프
 *
 * - 컴파일러 최적화 레벨(-O0 / -O2 등)에 무관하게 정확한 타이밍을 제공합니다.
 * - SystemCoreClock 값을 기준으로 사이클 수를 계산하므로 클럭 변경에도 자동 대응합니다.
 * - Cortex-M3 이상에서 사용 가능 (STM32F103: Cortex-M3).
 */

/** DWT 사이클 카운터 활성화. HAL_Init() 및 SystemClock_Config() 이후 1회 호출 */
void delayInit(void);

/** us 마이크로초 동안 busy-wait */
void delayUs(uint32_t us);

/** 현재 DWT 사이클 카운터 값 (32비트, 72MHz 기준 약 59초마다 wrap) */
static inline uint32_t delayGetCycles(void)
{
  return DWT->CYCCNT;
}

/** 두 사이클 값의 차이를 마이크로초로 환산 (wrap-around 안전) */
static inline uint32_t delayCyclesToUs(uint32_t start, uint32_t end)
{
  return (end - start) / (SystemCoreClock / 1000000U);
}

/** start 시점부터 현재까지 경과한 마이크로초 */
static inline uint32_t delayElapsedUs(uint32_t start)
{
  return delayCyclesToUs(start, DWT->CYCCNT);
}
