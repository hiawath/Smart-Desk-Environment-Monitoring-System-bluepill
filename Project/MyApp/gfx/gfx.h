#pragma once

#include <stdint.h>
#include <stdbool.h>

#define GFX_WIDTH       128
#define GFX_HEIGHT      64
#define GFX_BUF_SIZE    (GFX_WIDTH * GFX_HEIGHT / 8)

typedef enum {
  GFX_COLOR_BLACK = 0,
  GFX_COLOR_WHITE = 1
} gfxColor_t;

void gfxDrawPixel(uint8_t *buf, int16_t x, int16_t y, gfxColor_t color);
void gfxDrawLine(uint8_t *buf, int16_t x0, int16_t y0, int16_t x1, int16_t y1, gfxColor_t color);
void gfxDrawRect(uint8_t *buf, int16_t x, int16_t y, int16_t w, int16_t h, gfxColor_t color);
void gfxFillRect(uint8_t *buf, int16_t x, int16_t y, int16_t w, int16_t h, gfxColor_t color);
void gfxDrawChar(uint8_t *buf, int16_t x, int16_t y, char c, gfxColor_t color);
void gfxDrawString(uint8_t *buf, int16_t x, int16_t y, const char *str, gfxColor_t color);
