#include "bsp_gpio.h"
#include "bsp_timer.h"
#include "main.h"
#include "stm32f1xx_hal_gpio.h"

static uint8_t bgr_step = 0;

void ld2Toggle(void){
  HAL_GPIO_TogglePin(LD2_GPIO_Port,LD2_Pin);
}

void bgrSetColor(bool r, bool g, bool b)
{
  /* 공통 GND(Cathode)는 LOW 유지 */
  HAL_GPIO_WritePin(BGR_GND_GPIO_Port, BGR_GND_Pin, GPIO_PIN_RESET);

  /* R, G, B 제어 */
  HAL_GPIO_WritePin(BGR_R_GPIO_Port, BGR_R_Pin, r ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BGR_G_GPIO_Port, BGR_G_Pin, g ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BGR_B_GPIO_Port, BGR_B_Pin, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void bgrInit(void)
{
  bgr_step = 0;
  bgrSetColor(false, false, false); // 초기 상태: 모두 OFF
}

uint8_t bgrGetStep(void)
{
  return bgr_step;
}

/**
  * @brief  버튼 누름 1회당 BGR 순차 1사이클 진행
  *         Step 1: BGR_B ON
  *         Step 2: BGR_G ON
  *         Step 3: BGR_R ON
  *         Step 4: B, G, R 모두 ON
  *         Step 5: 모두 OFF (1사이클 완료) -> 다음 누르면 다시 Step 1
  */
void bgrNextStep(void)
{
  bgr_step++;
  if (bgr_step > 5)
  {
    bgr_step = 1;
  }

  switch (bgr_step)
  {
    case 1:
      /* BGR_B 켜기 */
      bgrSetColor(false, false, true);
      break;

    case 2:
      /* BGR_G 켜기 */
      bgrSetColor(false, true, false);
      break;

    case 3:
      /* BGR_R 켜기 */
      bgrSetColor(true, false, false);
      break;

    case 4:
      /* 모두 켜기 */
      bgrSetColor(true, true, true);
      break;

    case 5:
    default:
      /* 모두 끄기 */
      bgrSetColor(false, false, false);
      break;
  }
}

void gpioInit(void)
{
  bgrInit();
}

/**
  * @brief  EXTI Falling Edge 인터럽트 콜백 함수 (버튼 누름 감지)
  * @param  GPIO_Pin: 인터럽트 발생 핀
  */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_15)
  {
    /* 버튼 채터링(바운싱) 방지를 위한 소프트웨어 디바운스 (200ms) */
    static uint32_t last_press_tick = 0;
    uint32_t now = HAL_GetTick();

    if (now - last_press_tick >= 200)
    {
      last_press_tick = now;
      timerBreathingNextStep();
    }
  }
}

/**
  * @brief  EXTI Rising Edge 인터럽트 콜백 함수 (버튼 뗌 감지 등)
  * @param  GPIO_Pin: 인터럽트 발생 핀
  */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
  }
}

