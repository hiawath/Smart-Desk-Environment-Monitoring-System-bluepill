#include "bsp_adc.h"

/* STM32F103 데이터시트 표준 파라미터 (RM0008 기준) */
#define V25_MV                   1430.0f /* 25°C에서의 전압: 약 1.43V (1430mV) */
#define AVG_SLOPE                4.3f    /* 전압-온도 기울기: 4.3 mV/°C */
#define VREF_MV                  3300.0f /* ADC 기준 전압: 3.3V (VDDA) */
#define ADC_MAX_VAL              4095.0f /* 12비트 ADC 최대값 */

/* DMA 전송용 버퍼 (Half-Word / 16비트) */
static uint16_t adc_dma_buf = 0;

/* DMA 인터럽트와 메인 루프 간 공유 변수 */
static volatile uint32_t adc_raw_val = 0;
static volatile bool is_conv_done = false;

/* 계산된 결과 변수 (메인 컨텍스트에서 갱신) */
static float calculated_temp = 0.0f;
static bool temp_updated_flag = false;

static uint32_t sample_interval_ms = 500; /* 기본 샘플링 주기: 0.5초 (500ms) */
static uint32_t last_trigger_tick = 0;
static bool is_running = false;

void adcInit(void)
{
  adc_dma_buf = 0;
  adc_raw_val = 0;
  is_conv_done = false;
  calculated_temp = 0.0f;
  temp_updated_flag = false;
  sample_interval_ms = 500;
  is_running = false;

  /* STM32F103 필수: ADC 하드웨어 캘리브레이션 실행 */
  HAL_ADCEx_Calibration_Start(&hadc1);
}

/**
  * @brief  가변 주기로 DMA 기반 ADC 온도 샘플링 시작
  * @param  interval_ms: 샘플링 주기 (ms 단위, 0 입력 시 기본값 500ms(0.5초) 적용)
  */
void adcStartDMA(uint32_t interval_ms)
{
  if (interval_ms > 0)
  {
    sample_interval_ms = interval_ms;
  }
  else
  {
    sample_interval_ms = 500; /* 기본 0.5초 */
  }

  is_running = true;
  last_trigger_tick = HAL_GetTick() - sample_interval_ms; /* 즉시 첫 변환 시작 */
}

/**
  * @brief  샘플링 주기 변경
  * @param  interval_ms: 새 샘플링 주기 (ms)
  */
void adcSetInterval(uint32_t interval_ms)
{
  if (interval_ms > 0)
  {
    sample_interval_ms = interval_ms;
  }
}

/**
  * @brief  메인 루프에서 주기적으로 호출되어:
  *         1) DMA 완료 플래그 확인 시 STM32F103 공식으로 온도 계산
  *         2) 설정된 주기마다 DMA 변환 트리거
  */
void adcUpdate(void)
{
  if (!is_running)
    return;

  /* 1. DMA 완료 플래그가 설정된 경우 메인 컨텍스트에서 온도 계산 */
  if (is_conv_done)
  {
    is_conv_done = false;

    /* STM32F103 Reference Manual 공식:
       Temperature (°C) = {(V25 - VSENSE) / Avg_Slope} + 25
       (F103은 온도가 올라갈수록 전압이 낮아지는 음의 특성이므로 V25 - vsense) */
    float vsense_mv = ((float)adc_raw_val * VREF_MV) / ADC_MAX_VAL;
    calculated_temp = ((V25_MV - vsense_mv) / AVG_SLOPE) + 25.0f;

    temp_updated_flag = true;
  }

  /* 2. 설정된 주기(기본 0.5초)마다 다음 DMA 변환 트리거 */
  if (HAL_GetTick() - last_trigger_tick >= sample_interval_ms)
  {
    last_trigger_tick = HAL_GetTick();
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_dma_buf, 1);
  }
}

uint32_t adcGetRaw(void)
{
  return adc_raw_val;
}

float adcGetTemp(void)
{
  return calculated_temp;
}

bool adcIsUpdated(void)
{
  if (temp_updated_flag)
  {
    temp_updated_flag = false;
    return true;
  }
  return false;
}

/**
  * @brief  ADC DMA 변환 완료 시 호출되는 콜백 함수 (ISR 컨텍스트)
  *         DMA 버퍼 데이터 저장 및 flag 설정만 수행
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  if (hadc->Instance == ADC1)
  {
    adc_raw_val = adc_dma_buf;
    is_conv_done = true; /* 변환 완료 플래그 설정 */
    HAL_ADC_Stop_DMA(hadc); /* 다음 Start_DMA가 HAL_BUSY에 걸리지 않도록 정지 */
  }
}

