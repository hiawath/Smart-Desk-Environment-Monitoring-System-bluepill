#pragma once

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

void gpioInit(void);
void bgrInit(void);
void bgrNextStep(void);
void bgrSetColor(bool r, bool g, bool b);
uint8_t bgrGetStep(void);
void ld2Toggle(void);
