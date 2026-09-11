#pragma once
#include "main.h"
#include "tim.h"
#include <stdint.h>

typedef enum{
    LED_1=0,
    LED_2,
    LED_3,

    LED_MAX_COUNT
} ledId_t;
typedef enum {
    BREATH_MODE_OFF = 0,   /* 모두 OFF */
    BREATH_MODE_BLUE,      /* 1단계: BGR_B (LED_1) */
    BREATH_MODE_GREEN,     /* 2단계: BGR_G (LED_2) */
    BREATH_MODE_RED,       /* 3단계: BGR_R (LED_3) */
    BREATH_MODE_FULL,      /* 4단계: B, G, R 전체 */
    BREATH_MODE_MAX
} breathMode_t;

void timerInit(void);
void timerPwmStart(void);
void timerPwmStop(void);

void timerSetDuty(ledId_t id, float duty_percent);
uint8_t timerGetDuty(ledId_t id);

void timerSetDutyFloat(float duty_percent);
void timerLedUpdate(void);

/* Breathing 모드 5단계 제어 함수 */
void timerSetBreathingMode(breathMode_t mode);
void timerBreathingNextStep(void);
breathMode_t timerGetBreathingMode(void);