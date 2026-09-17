#pragma once

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define SSD1306_SPI_WIDTH   128
#define SSD1306_SPI_HEIGHT  64

#define SSD1306_SPI_COLOR_BLACK  0
#define SSD1306_SPI_COLOR_WHITE  1
#define SSD1306_SPI_COLOR_YELLOW 1  /* 옐로우-블루 투톤 패널 상단 영역 (Y: 0 ~ 15) */
#define SSD1306_SPI_COLOR_BLUE   1  /* 옐로우-블루 투톤 패널 하단 영역 (Y: 16 ~ 63) */

/* 객체지향 핸들러 타입 전방 선언 */
typedef struct ssd1306SpiHandle_s ssd1306SpiHandle_t;

struct ssd1306SpiHandle_s {
  bool (*init)(void);
  void (*clear)(void);
  void (*update)(void);
  void (*drawPixel)(int16_t x, int16_t y, uint8_t color);
  void (*drawLine)(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
  void (*drawRect)(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
  void (*fillRect)(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
  void (*drawChar)(int16_t x, int16_t y, char ch, uint8_t color);
  void (*drawString)(int16_t x, int16_t y, const char *str, uint8_t color);
  void (*test)(void);
};

/* 전역 SPI SSD1306 핸들러 인스턴스 */
extern ssd1306SpiHandle_t hssd1306Spi;

/* 기본 함수형 API */
bool ssd1306SpiInit(void);
void ssd1306SpiClear(void);
void ssd1306SpiUpdate(void);
void ssd1306SpiDrawPixel(int16_t x, int16_t y, uint8_t color);
void ssd1306SpiDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
void ssd1306SpiDrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void ssd1306SpiFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void ssd1306SpiDrawChar(int16_t x, int16_t y, char ch, uint8_t color);
void ssd1306SpiDrawString(int16_t x, int16_t y, const char *str, uint8_t color);
void ssd1306SpiTest(void);
uint8_t *ssd1306SpiGetBuffer(void);
