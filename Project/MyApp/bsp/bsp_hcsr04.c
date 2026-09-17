#include "bsp_hcsr04.h"
#include "bsp_delay.h"

static hcSr04Handle_t *s_active_hc = NULL;

/* ----------------------------------------------------------------
 * 객체 지향 메서드 래퍼 함수들 (hcSr04_ 네임스페이스)
 * ----------------------------------------------------------------*/

static bool hcSr04_read(float *distance_cm)
{
  return hcSr04Read(s_active_hc, distance_cm);
}

static float hcSr04_getDistance(void)
{
  return hcSr04GetDistance(s_active_hc);
}

/* ----------------------------------------------------------------
 * 공개 API
 * ----------------------------------------------------------------*/

void hcSr04Init(hcSr04Handle_t *hhc, const hcSr04Pin_t *pins)
{
  if (!hhc || !pins)
    return;

  hhc->pins = *pins;

  hhc->latest_distance = 0.0f;
  hhc->fail_count      = 0;
  hhc->initialized     = false;

  s_active_hc          = hhc;

  /* 메서드 함수 포인터 바인딩 */
  hhc->read        = hcSr04_read;
  hhc->getDistance = hcSr04_getDistance;

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Trig Pin: Output Push-Pull */
  GPIO_InitStruct.Pin   = hhc->pins.trig_pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(hhc->pins.trig_port, &GPIO_InitStruct);

  /* Echo Pin: Input with Pull-Down */
  GPIO_InitStruct.Pin   = hhc->pins.echo_pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  HAL_GPIO_Init(hhc->pins.echo_port, &GPIO_InitStruct);

  HAL_GPIO_WritePin(hhc->pins.trig_port, hhc->pins.trig_pin, GPIO_PIN_RESET);
  hhc->initialized = true;
}

/**
  * @brief  HC-SR04 초음파 센서로 거리를 측정 (단위: cm)
  * @param  hhc: HC-SR04 핸들 포인터
  * @param  distance_cm: 측정된 거리 값을 저장할 포인터
  * @retval true: 성공, false: 측정 실패(범위 초과/타임아웃)
  */
bool hcSr04Read(hcSr04Handle_t *hhc, float *distance_cm)
{
  if (!hhc || !hhc->initialized)
    return false;

  /* 0. 잔류 Echo 신호가 있으면 LOW가 될 때까지 대기 (최대 3ms) */
  uint32_t wait_low = 1500;
  while (HAL_GPIO_ReadPin(hhc->pins.echo_port, hhc->pins.echo_pin) == GPIO_PIN_SET)
  {
    delayUs(2);
    if (--wait_low == 0)
    {
      if (++hhc->fail_count > 4 && distance_cm) *distance_cm = 0.0f;
      return false;
    }
  }

  /* 1. Trig 핀에 10us HIGH 펄스 인가 */
  HAL_GPIO_WritePin(hhc->pins.trig_port, hhc->pins.trig_pin, GPIO_PIN_RESET);
  delayUs(2);
  HAL_GPIO_WritePin(hhc->pins.trig_port, hhc->pins.trig_pin, GPIO_PIN_SET);
  delayUs(10);
  HAL_GPIO_WritePin(hhc->pins.trig_port, hhc->pins.trig_pin, GPIO_PIN_RESET);

  /* 2. Echo 핀이 HIGH가 될 때까지 대기 (최대 6ms 타임아웃) */
  uint32_t timeout = 3000;
  while (HAL_GPIO_ReadPin(hhc->pins.echo_port, hhc->pins.echo_pin) == GPIO_PIN_RESET)
  {
    delayUs(2);
    if (--timeout == 0)
    {
      if (++hhc->fail_count > 4 && distance_cm) *distance_cm = 0.0f;
      return false;
    }
  }

  /* 3. Echo 핀 HIGH 지속 시간 측정 (DWT 사이클 카운터 기반 정밀 측정, 최대 25ms ≈ 430cm) */
  uint32_t start_cycles = delayGetCycles();
  uint32_t max_cycles = 25000U * (SystemCoreClock / 1000000U);
  while (HAL_GPIO_ReadPin(hhc->pins.echo_port, hhc->pins.echo_pin) == GPIO_PIN_SET)
  {
    if ((delayGetCycles() - start_cycles) > max_cycles)
    {
      if (++hhc->fail_count > 4 && distance_cm) *distance_cm = 0.0f;
      return false;
    }
  }

  /* 4. 거리(cm) 환산 (음속 340m/s: 시간(us) / 58.0) */
  uint32_t duration_us = delayCyclesToUs(start_cycles, delayGetCycles());
  float dist = (float)duration_us / 58.0f;

  /* 유효 거리 범위 체크 (2cm ~ 400cm) */
  if (dist >= 2.0f && dist <= 400.0f)
  {
    hhc->fail_count = 0;
    hhc->latest_distance = dist;
    if (distance_cm)
    {
      *distance_cm = dist;
    }
    return true;
  }

  if (++hhc->fail_count > 4 && distance_cm)
  {
    *distance_cm = 0.0f;
  }
  return false;
}

float hcSr04GetDistance(hcSr04Handle_t *hhc)
{
  return hhc ? hhc->latest_distance : 0.0f;
}

