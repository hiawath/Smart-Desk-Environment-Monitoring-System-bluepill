#pragma once

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* HC-SR04 핀 구조체 */
typedef struct {
  GPIO_TypeDef *trig_port;
  uint16_t      trig_pin;
  GPIO_TypeDef *echo_port;
  uint16_t      echo_pin;
} hcSr04Pin_t;

/* HC-SR04 드라이버 핸들 구조체 (객체 지향 메서드 포함) */
typedef struct {
  hcSr04Pin_t pins;
  float       latest_distance;
  uint8_t     fail_count;
  bool        initialized;

  /* 객체 지향 메서드 */
  bool  (*read)(float *distance_cm);
  float (*getDistance)(void);
} hcSr04Handle_t;

void  hcSr04Init(hcSr04Handle_t *hhc, const hcSr04Pin_t *pins);
bool  hcSr04Read(hcSr04Handle_t *hhc, float *distance_cm);
float hcSr04GetDistance(hcSr04Handle_t *hhc);

