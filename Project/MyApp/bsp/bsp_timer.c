#include "bsp_timer.h"

#include "main.h"

#include "tim.h"
#include <stdbool.h>
#include <stdint.h>

static uint8_t s_current_duty = 50;

typedef struct _ledChannel_t {
  TIM_HandleTypeDef *htim;
  uint32_t channel;
  uint32_t period_ms;
  uint32_t offset_ms;
  bool breath_enable;
  uint8_t current_duty;
  bool is_n_channel; /* N(Complementary) 채널 여부: true 시
                        HAL_TIMEx_PWMN_Start/Stop 사용 */
  bool invert_duty;  /* 신호가 반대로 출력되는 경우 듀티 보정 */
} ledChannel_t;

static ledChannel_t s_led_table[LED_MAX_COUNT] = {
    [LED_1] = {&htim2, TIM_CHANNEL_4, 3000,    0, true, 0, false, false}, /* BGR_B: PB10 (TIM8 CH1 - 일반 PWM 동작) */
    [LED_2] = {&htim2, TIM_CHANNEL_3, 3000, 1000, true, 0, true,  false}, /* BGR_G: PB2 (TIM8 CH4 - 보수/특수 채널 HAL_TIMEx_PWMN_Start 동작) */
    [LED_3] = {&htim3, TIM_CHANNEL_4, 3000, 2000, true, 0, false, false}, /* BGR_R: PB1 (TIM3 CH4 - 일반 PWM 동작) */
};

void timerInit(void) {
  /* 1. 타이머 및 GPIOB 클럭 활성화 */
  __HAL_RCC_TIM3_CLK_ENABLE();
  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* 2. PB10, PB2, PB1 핀을 타이머 PWM Alternate Function(AF) 모드로 재설정 */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

  /* PB1 (BGR_R) -> TIM3_CH4 (AF2) */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* PB10 (BGR_G) -> TIM2_CH3 (AF3) */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* PB11 (BGR_B) -> TIM2_CH4 (AF2) */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* BGR 공통 GND(Cathode) 핀 LOW 유지 */
  HAL_GPIO_WritePin(BGR_GND_GPIO_Port, BGR_GND_Pin, GPIO_PIN_RESET);

  /* 3. TIM3 및 TIM8의 PWM 채널 초기화 */
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* TIM3 Channel 4 설정 */
  HAL_TIM_PWM_Init(&htim3);
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4);

  /* TIM8 Channel 1 (PB10 - TIM8_CH1 메인 채널 설정) */
  HAL_TIM_PWM_Init(&htim2);
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3);

  /* TIM8 Channel 4 (PB2 - BGR_G) */
  HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4);

  /* TIM8 고급 타이머: 메인 출력 인에이블(MOE) 필수 */
  __HAL_TIM_ENABLE(&htim2);
  __HAL_TIM_ENABLE(&htim3);

  /* 4. PWM 시작 및 초기 Breathing 대기(OFF) 상태 설정 */
  timerPwmStart();
  timerSetBreathingMode(BREATH_MODE_OFF);
}

void timerPwmStart(void) {
  /* LED_1 (TIM8 CH1 - PB10): 일반 메인 채널 호출 */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

  /* LED_2 (TIM8 CH4 - PB2): 보수/특수 N-채널 전용 호출 */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

  /* LED_3 (TIM3 CH4 - PB1): 일반 PWM 호출 */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

  /* 카운터 및 MOE 확실히 인에이블 */
  __HAL_TIM_ENABLE(&htim2);
  __HAL_TIM_ENABLE(&htim3);
}

void timerPwmStop(void) {
  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
}

void timerSetDuty(ledId_t id, float duty_percent) {
  if (id >= LED_MAX_COUNT || s_led_table[id].htim == NULL) {
    return;
  }

  if (duty_percent < 0.0f)
    duty_percent = 0.0f;
  if (duty_percent > 100.0f)
    duty_percent = 100.0f;

  s_led_table[id].current_duty = (uint8_t)(duty_percent + 0.5f);

  /* ARR (Auto-reload register) 값 가져오기 (+1 하여 전체 주기 계산) */
  uint32_t period = __HAL_TIM_GET_AUTORELOAD(s_led_table[id].htim) + 1;
  uint32_t pulse = (uint32_t)(((float)period * duty_percent) / 100.0f);

  if (s_led_table[id].invert_duty) {
    pulse = (period > pulse) ? (period - pulse) : 0;
  }

  __HAL_TIM_SET_COMPARE(s_led_table[id].htim, s_led_table[id].channel, pulse);
}


void timerSetDutyFloat(float duty_percent) {
  if (duty_percent < 0.0f)
    duty_percent = 0.0f;
  if (duty_percent > 100.0f)
    duty_percent = 100.0f;

  s_current_duty = (uint8_t)(duty_percent + 0.5f);

  for (int i = 0; i < LED_MAX_COUNT; i++) {
    timerSetDuty((ledId_t)i, duty_percent);
  }
}

uint8_t timerGetDuty(ledId_t id) {
  if (id < LED_MAX_COUNT) {
    return s_led_table[id].current_duty;
  }
  return 0;
}

static float calculateBreathDuty(uint32_t elapsed_time, uint32_t period_ms) {
  if (period_ms == 0)
    return 0.0f;

  uint32_t hal_period = period_ms / 2;
  float normalized_progress; //

  if (elapsed_time < hal_period) {
    normalized_progress = (float)elapsed_time / (float)hal_period;
  } else {
    normalized_progress =
        1.0f - ((float)(elapsed_time - hal_period) / (float)hal_period);
  }

  return (normalized_progress * normalized_progress * 100.0f);
}

void timerLedUpdate(void) {
  static uint32_t last_tick = 0;
  static uint32_t elapsed_time = 0;

  uint32_t now = HAL_GetTick();
  uint32_t dt = now - last_tick;

  if (dt >= 10) {
    last_tick = now;
    elapsed_time += dt;

    for (int i = 0; i < LED_MAX_COUNT; i++) {
      if (!s_led_table[i].breath_enable || s_led_table[i].period_ms == 0) {
        continue;
      }

      uint32_t t =
          (elapsed_time + s_led_table[i].offset_ms) % s_led_table[i].period_ms;
      float duty = calculateBreathDuty(t, s_led_table[i].period_ms);
      timerSetDuty((ledId_t)i, duty);
    }
  }
}

/* ----------------------------------------------------------------
 * Breathing 5단계 모드 제어 구현 (B -> G -> R -> Full -> Off)
 * ----------------------------------------------------------------*/

static breathMode_t s_current_breath_mode = BREATH_MODE_OFF;

void timerSetBreathingMode(breathMode_t mode)
{
    s_current_breath_mode = mode;

    switch (mode)
    {
    case BREATH_MODE_BLUE:
        /* 1단계: BGR_B (LED_1)만 Breathing */
        s_led_table[LED_1].breath_enable = true;
        s_led_table[LED_2].breath_enable = false;
        s_led_table[LED_3].breath_enable = false;
        timerSetDuty(LED_2, 0.0f);
        timerSetDuty(LED_3, 0.0f);
        break;

    case BREATH_MODE_GREEN:
        /* 2단계: BGR_G (LED_2)만 Breathing */
        s_led_table[LED_1].breath_enable = false;
        s_led_table[LED_2].breath_enable = true;
        s_led_table[LED_3].breath_enable = false;
        timerSetDuty(LED_1, 0.0f);
        timerSetDuty(LED_3, 0.0f);
        break;

    case BREATH_MODE_RED:
        /* 3단계: BGR_R (LED_3)만 Breathing */
        s_led_table[LED_1].breath_enable = false;
        s_led_table[LED_2].breath_enable = false;
        s_led_table[LED_3].breath_enable = true;
        timerSetDuty(LED_1, 0.0f);
        timerSetDuty(LED_2, 0.0f);
        break;

    case BREATH_MODE_FULL:
        /* 4단계: B, G, R 전체 동시 Breathing */
        s_led_table[LED_1].breath_enable = true;
        s_led_table[LED_2].breath_enable = true;
        s_led_table[LED_3].breath_enable = true;
        break;

    case BREATH_MODE_OFF:
    default:
        /* 5단계: 모두 끄기 (Breathing 중지) */
        s_led_table[LED_1].breath_enable = false;
        s_led_table[LED_2].breath_enable = false;
        s_led_table[LED_3].breath_enable = false;
        timerSetDuty(LED_1, 0.0f);
        timerSetDuty(LED_2, 0.0f);
        timerSetDuty(LED_3, 0.0f);
        break;
    }
}

/**
 * @brief PA15 버튼 누름 시 호출: B -> G -> R -> Full -> Off 5단계 순환
 */
void timerBreathingNextStep(void)
{
    breathMode_t next_mode;

    switch (s_current_breath_mode)
    {
    case BREATH_MODE_OFF:
        next_mode = BREATH_MODE_BLUE;
        break;
    case BREATH_MODE_BLUE:
        next_mode = BREATH_MODE_GREEN;
        break;
    case BREATH_MODE_GREEN:
        next_mode = BREATH_MODE_RED;
        break;
    case BREATH_MODE_RED:
        next_mode = BREATH_MODE_FULL;
        break;
    case BREATH_MODE_FULL:
        next_mode = BREATH_MODE_OFF;
        break;
    default:
        next_mode = BREATH_MODE_BLUE;
        break;
    }

    timerSetBreathingMode(next_mode);
}

breathMode_t timerGetBreathingMode(void)
{
    return s_current_breath_mode;
}

