#include "bsp_ssd1306_spi.h"
#include "gfx/gfx.h"
#include <stdio.h>
#include <string.h>

static uint8_t ssd1306_spi_buffer[SSD1306_SPI_WIDTH * SSD1306_SPI_HEIGHT / 8];

uint8_t *ssd1306SpiGetBuffer(void)
{
  return ssd1306_spi_buffer;
}

/* ---------------- 고속 Bit-Banging SPI 전송 함수 ---------------- */

static inline void ssd1306SpiTransmitByte(uint8_t byte)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    HAL_GPIO_WritePin(SSD1306_SCK_GPIO_Port, SSD1306_SCK_Pin, GPIO_PIN_RESET);
    if (byte & 0x80) {
      HAL_GPIO_WritePin(SSD1306_MOSI_GPIO_Port, SSD1306_MOSI_Pin, GPIO_PIN_SET);
    } else {
      HAL_GPIO_WritePin(SSD1306_MOSI_GPIO_Port, SSD1306_MOSI_Pin, GPIO_PIN_RESET);
    }
    HAL_GPIO_WritePin(SSD1306_SCK_GPIO_Port, SSD1306_SCK_Pin, GPIO_PIN_SET);
    byte <<= 1;
  }
}

static void ssd1306SpiWriteCommand(const uint8_t *cmd, uint16_t len)
{
  HAL_GPIO_WritePin(SSD1306_DC_GPIO_Port, SSD1306_DC_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SSD1306_CS_GPIO_Port, SSD1306_CS_Pin, GPIO_PIN_RESET);

  for (uint16_t i = 0; i < len; i++)
  {
    ssd1306SpiTransmitByte(cmd[i]);
  }

  HAL_GPIO_WritePin(SSD1306_CS_GPIO_Port, SSD1306_CS_Pin, GPIO_PIN_SET);
}

static void ssd1306SpiWriteData(const uint8_t *data, uint16_t len)
{
  HAL_GPIO_WritePin(SSD1306_DC_GPIO_Port, SSD1306_DC_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SSD1306_CS_GPIO_Port, SSD1306_CS_Pin, GPIO_PIN_RESET);

  for (uint16_t i = 0; i < len; i++)
  {
    ssd1306SpiTransmitByte(data[i]);
  }

  HAL_GPIO_WritePin(SSD1306_CS_GPIO_Port, SSD1306_CS_Pin, GPIO_PIN_SET);
}

/* ---------------- 초기화 및 기본 제어 API ---------------- */

bool ssd1306SpiInit(void)
{
  printf("\r\n========================================\r\n");
  printf("[SSD1306_SPI] Initializing 7-Pin SPI OLED...\r\n");

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

  /* CS(PA4), SCK(PA5), DC(PA6), MOSI(PA7), RES(PA1) */
  GPIO_InitStruct.Pin = SSD1306_CS_Pin | SSD1306_SCK_Pin | SSD1306_DC_Pin | SSD1306_MOSI_Pin | SSD1306_RES_Pin;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(SSD1306_CS_GPIO_Port, SSD1306_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SSD1306_SCK_GPIO_Port, SSD1306_SCK_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SSD1306_DC_GPIO_Port, SSD1306_DC_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SSD1306_MOSI_GPIO_Port, SSD1306_MOSI_Pin, GPIO_PIN_RESET);

  /* 하드웨어 리셋 */
  HAL_GPIO_WritePin(SSD1306_RES_GPIO_Port, SSD1306_RES_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(SSD1306_RES_GPIO_Port, SSD1306_RES_Pin, GPIO_PIN_SET);
  HAL_Delay(50);

  /* SSD1306 초기화 커맨드 시퀀스 */
  static const uint8_t init_sequence[] = {
    0xAE,             /* Display OFF */
    0xD5, 0x80,       /* Set Display Clock Divide Ratio */
    0xA8, 0x3F,       /* Set Multiplex Ratio (1/64) */
    0xD3, 0x00,       /* Set Display Offset */
    0x40,             /* Set Start Line (0) */
    0xAD, 0x8B,       /* SH1106: DC-DC ON */
    0x8D, 0x14,       /* SSD1306: Enable Charge Pump (7.5V) */
    0x20, 0x02,       /* Page Addressing Mode */
    0xA1,             /* Segment Re-map */
    0xC8,             /* COM Output Scan Direction */
    0xDA, 0x12,       /* Set COM Pins Config */
    0x81, 0xCF,       /* Set Contrast */
    0xD9, 0xF1,       /* Set Pre-charge Period */
    0xDB, 0x40,       /* Set VCOMH Deselect Level */
    0xA4,             /* Output follows RAM */
    0xA6,             /* Normal Display */
    0xAF              /* Display ON */
  };

  ssd1306SpiWriteCommand(init_sequence, sizeof(init_sequence));
  HAL_Delay(20);

  ssd1306SpiClear();
  ssd1306SpiUpdate();
  printf("[SSD1306_SPI] Init Success! Ready.\r\n");
  printf("========================================\r\n\r\n");

  return true;
}

void ssd1306SpiClear(void)
{
  memset(ssd1306_spi_buffer, 0x00, sizeof(ssd1306_spi_buffer));
}

void ssd1306SpiUpdate(void)
{
  for (uint8_t page = 0; page < 8; page++)
  {
    uint8_t page_cmd[3];
    page_cmd[0] = 0xB0 + page; /* Page 0~7 */
    page_cmd[1] = 0x00;        /* Column Lower Nibble = 0 */
    page_cmd[2] = 0x10;        /* Column Higher Nibble = 0 */

    ssd1306SpiWriteCommand(page_cmd, sizeof(page_cmd));
    ssd1306SpiWriteData(&ssd1306_spi_buffer[page * SSD1306_SPI_WIDTH], SSD1306_SPI_WIDTH);
  }
}

/* ---------------- GFX 프리미티브 래퍼 구현 ---------------- */

void ssd1306SpiDrawPixel(int16_t x, int16_t y, uint8_t color)
{
  gfxDrawPixel(ssd1306_spi_buffer, x, y, (gfxColor_t)color);
}

void ssd1306SpiDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
  gfxDrawLine(ssd1306_spi_buffer, x0, y0, x1, y1, (gfxColor_t)color);
}

void ssd1306SpiDrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
  gfxDrawRect(ssd1306_spi_buffer, x, y, w, h, (gfxColor_t)color);
}

void ssd1306SpiFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
  gfxFillRect(ssd1306_spi_buffer, x, y, w, h, (gfxColor_t)color);
}

void ssd1306SpiDrawChar(int16_t x, int16_t y, char ch, uint8_t color)
{
  gfxDrawChar(ssd1306_spi_buffer, x, y, ch, (gfxColor_t)color);
}

void ssd1306SpiDrawString(int16_t x, int16_t y, const char *str, uint8_t color)
{
  gfxDrawString(ssd1306_spi_buffer, x, y, str, (gfxColor_t)color);
}

void ssd1306SpiTest(void)
{
  /* 테스트 패턴 */
  ssd1306SpiClear();
  ssd1306SpiDrawRect(0, 0, SSD1306_SPI_WIDTH, SSD1306_SPI_HEIGHT, SSD1306_SPI_COLOR_WHITE);
  ssd1306SpiDrawString(10, 10, "GFX Lib OK!", SSD1306_SPI_COLOR_WHITE);
  ssd1306SpiUpdate();
}

/* ---------------- 전역 핸들러 인스턴스 ---------------- */

ssd1306SpiHandle_t hssd1306Spi = {
  .init       = ssd1306SpiInit,
  .clear      = ssd1306SpiClear,
  .update     = ssd1306SpiUpdate,
  .drawPixel  = ssd1306SpiDrawPixel,
  .drawLine   = ssd1306SpiDrawLine,
  .drawRect   = ssd1306SpiDrawRect,
  .fillRect   = ssd1306SpiFillRect,
  .drawChar   = ssd1306SpiDrawChar,
  .drawString = ssd1306SpiDrawString,
  .test       = ssd1306SpiTest
};
