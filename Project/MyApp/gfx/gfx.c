#include "gfx.h"
#include "font6x8.h"
#include <stdlib.h>

void gfxDrawPixel(uint8_t *buf, int16_t x, int16_t y, gfxColor_t color)
{
  if (!buf || x < 0 || x >= GFX_WIDTH || y < 0 || y >= GFX_HEIGHT)
    return;

  uint16_t index = (uint16_t)(x + (y / 8) * GFX_WIDTH);
  uint8_t bit = (uint8_t)(1 << (y % 8));

  if (color == GFX_COLOR_WHITE)
  {
    buf[index] |= bit;
  }
  else
  {
    buf[index] &= ~bit;
  }
}

void gfxDrawLine(uint8_t *buf, int16_t x0, int16_t y0, int16_t x1, int16_t y1, gfxColor_t color)
{
  int16_t dx = (int16_t)abs(x1 - x0);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t dy = (int16_t)-abs(y1 - y0);
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = (int16_t)(dx + dy);

  while (1)
  {
    gfxDrawPixel(buf, x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    int16_t e2 = (int16_t)(2 * err);
    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

void gfxDrawRect(uint8_t *buf, int16_t x, int16_t y, int16_t w, int16_t h, gfxColor_t color)
{
  gfxDrawLine(buf, x, y, (int16_t)(x + w - 1), y, color);
  gfxDrawLine(buf, x, (int16_t)(y + h - 1), (int16_t)(x + w - 1), (int16_t)(y + h - 1), color);
  gfxDrawLine(buf, x, y, x, (int16_t)(y + h - 1), color);
  gfxDrawLine(buf, (int16_t)(x + w - 1), y, (int16_t)(x + w - 1), (int16_t)(y + h - 1), color);
}

void gfxFillRect(uint8_t *buf, int16_t x, int16_t y, int16_t w, int16_t h, gfxColor_t color)
{
  for (int16_t i = x; i < x + w; i++)
  {
    for (int16_t j = y; j < y + h; j++)
    {
      gfxDrawPixel(buf, i, j, color);
    }
  }
}

void gfxDrawChar(uint8_t *buf, int16_t x, int16_t y, char c, gfxColor_t color)
{
  if (c < ' ' || c > '~')
    return;

  uint8_t char_index = (uint8_t)(c - ' ');
  for (uint8_t col = 0; col < 6; col++)
  {
    uint8_t line = font6x8[char_index][col];
    for (uint8_t row = 0; row < 8; row++)
    {
      if (line & 0x01)
      {
        gfxDrawPixel(buf, (int16_t)(x + col), (int16_t)(y + row), color);
      }
      else
      {
        gfxDrawPixel(buf, (int16_t)(x + col), (int16_t)(y + row),
                     (color == GFX_COLOR_WHITE) ? GFX_COLOR_BLACK : GFX_COLOR_WHITE);
      }
      line >>= 1;
    }
  }
}

void gfxDrawString(uint8_t *buf, int16_t x, int16_t y, const char *str, gfxColor_t color)
{
  if (!str)
    return;

  while (*str)
  {
    gfxDrawChar(buf, x, y, *str++, color);
    x += 6;
  }
}
