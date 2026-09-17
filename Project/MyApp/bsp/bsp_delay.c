#include "bsp_delay.h"

void delayInit(void)
{
  /* Cortex-M3 CoreDebug 및 DWT 사이클 카운터 활성화 */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

void delayUs(uint32_t us)
{
  uint32_t start  = DWT->CYCCNT;
  uint32_t cycles = us * (SystemCoreClock / 1000000U);
  while ((DWT->CYCCNT - start) < cycles)
  {
    __NOP();
  }
}
