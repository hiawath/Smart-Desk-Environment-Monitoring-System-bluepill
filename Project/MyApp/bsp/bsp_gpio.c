#include "bsp_gpio.h"

void ld2Toggle(void)
{
  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}

void gpioInit(void)
{
  /* 온보드 LD2 초기 상태: OFF (STM32F103 Blue Pill의 PC13은 Active LOW이므로 SET=OFF) */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
}
