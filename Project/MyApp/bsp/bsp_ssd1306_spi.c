#include "bsp_ssd1306_spi.h"
#include "spi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ---------------- 객체지향 인스턴스 바인딩 ---------------- */

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

static uint8_t ssd1306_spi_buffer[SSD1306_SPI_WIDTH * SSD1306_SPI_HEIGHT / 8];

/* 6x8 Basic ASCII Font Table (ASCII 0x20 ' ' to 0x7E '~') */
static const uint8_t font6x8[][6] = {
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // ' '
  { 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00 }, // '!'
  { 0x00, 0x07, 0x00, 0x07, 0x00, 0x00 }, // '"'
  { 0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00 }, // '#'
  { 0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00 }, // '$'
  { 0x23, 0x13, 0x08, 0x64, 0x62, 0x00 }, // '%'
  { 0x36, 0x49, 0x55, 0x22, 0x50, 0x00 }, // '&'
  { 0x00, 0x05, 0x03, 0x00, 0x00, 0x00 }, // '''
  { 0x00, 0x1C, 0x22, 0x41, 0x00, 0x00 }, // '('
  { 0x00, 0x41, 0x22, 0x1C, 0x00, 0x00 }, // ')'
  { 0x08, 0x2A, 0x1C, 0x2A, 0x08, 0x00 }, // '*'
  { 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00 }, // '+'
  { 0x00, 0x50, 0x30, 0x00, 0x00, 0x00 }, // ','
  { 0x08, 0x08, 0x08, 0x08, 0x08, 0x00 }, // '-'
  { 0x00, 0x60, 0x60, 0x00, 0x00, 0x00 }, // '.'
  { 0x20, 0x10, 0x08, 0x04, 0x02, 0x00 }, // '/'
  { 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00 }, // '0'
  { 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00 }, // '1'
  { 0x42, 0x61, 0x51, 0x49, 0x46, 0x00 }, // '2'
  { 0x21, 0x41, 0x45, 0x4B, 0x31, 0x00 }, // '3'
  { 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00 }, // '4'
  { 0x27, 0x45, 0x45, 0x45, 0x39, 0x00 }, // '5'
  { 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00 }, // '6'
  { 0x01, 0x71, 0x09, 0x05, 0x03, 0x00 }, // '7'
  { 0x36, 0x49, 0x49, 0x49, 0x36, 0x00 }, // '8'
  { 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00 }, // '9'
  { 0x00, 0x36, 0x36, 0x00, 0x00, 0x00 }, // ':'
  { 0x00, 0x56, 0x36, 0x00, 0x00, 0x00 }, // ';'
  { 0x00, 0x08, 0x14, 0x22, 0x41, 0x00 }, // '<'
  { 0x14, 0x14, 0x14, 0x14, 0x14, 0x00 }, // '='
  { 0x00, 0x41, 0x22, 0x14, 0x08, 0x00 }, // '>'
  { 0x02, 0x01, 0x51, 0x09, 0x06, 0x00 }, // '?'
  { 0x32, 0x49, 0x79, 0x41, 0x3E, 0x00 }, // '@'
  { 0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00 }, // 'A'
  { 0x7F, 0x49, 0x49, 0x49, 0x36, 0x00 }, // 'B'
  { 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00 }, // 'C'
  { 0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00 }, // 'D'
  { 0x7F, 0x49, 0x49, 0x49, 0x41, 0x00 }, // 'E'
  { 0x7F, 0x09, 0x09, 0x01, 0x01, 0x00 }, // 'F'
  { 0x3E, 0x41, 0x41, 0x51, 0x32, 0x00 }, // 'G'
  { 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00 }, // 'H'
  { 0x00, 0x41, 0x7F, 0x41, 0x00, 0x00 }, // 'I'
  { 0x20, 0x40, 0x41, 0x3F, 0x01, 0x00 }, // 'J'
  { 0x7F, 0x08, 0x14, 0x22, 0x41, 0x00 }, // 'K'
  { 0x7F, 0x40, 0x40, 0x40, 0x40, 0x00 }, // 'L'
  { 0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00 }, // 'M'
  { 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00 }, // 'N'
  { 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00 }, // 'O'
  { 0x7F, 0x09, 0x09, 0x09, 0x06, 0x00 }, // 'P'
  { 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00 }, // 'Q'
  { 0x7F, 0x09, 0x19, 0x29, 0x46, 0x00 }, // 'R'
  { 0x46, 0x49, 0x49, 0x49, 0x31, 0x00 }, // 'S'
  { 0x01, 0x01, 0x7F, 0x01, 0x01, 0x00 }, // 'T'
  { 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00 }, // 'U'
  { 0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00 }, // 'V'
  { 0x7F, 0x20, 0x18, 0x20, 0x7F, 0x00 }, // 'W'
  { 0x63, 0x14, 0x08, 0x14, 0x63, 0x00 }, // 'X'
  { 0x03, 0x04, 0x78, 0x04, 0x03, 0x00 }, // 'Y'
  { 0x61, 0x51, 0x49, 0x45, 0x43, 0x00 }, // 'Z'
  { 0x00, 0x7F, 0x41, 0x41, 0x00, 0x00 }, // '['
  { 0x02, 0x04, 0x08, 0x10, 0x20, 0x00 }, // '\'
  { 0x00, 0x41, 0x41, 0x7F, 0x00, 0x00 }, // ']'
  { 0x04, 0x02, 0x01, 0x02, 0x04, 0x00 }, // '^'
  { 0x40, 0x40, 0x40, 0x40, 0x40, 0x00 }, // '_'
  { 0x00, 0x01, 0x02, 0x04, 0x00, 0x00 }, // '`'
  { 0x20, 0x54, 0x54, 0x54, 0x78, 0x00 }, // 'a'
  { 0x7F, 0x48, 0x44, 0x44, 0x38, 0x00 }, // 'b'
  { 0x38, 0x44, 0x44, 0x44, 0x20, 0x00 }, // 'c'
  { 0x38, 0x44, 0x44, 0x48, 0x7F, 0x00 }, // 'd'
  { 0x38, 0x54, 0x54, 0x54, 0x18, 0x00 }, // 'e'
  { 0x08, 0x7E, 0x09, 0x01, 0x02, 0x00 }, // 'f'
  { 0x18, 0xA4, 0xA4, 0xA4, 0x7C, 0x00 }, // 'g'
  { 0x7F, 0x08, 0x04, 0x04, 0x78, 0x00 }, // 'h'
  { 0x00, 0x44, 0x7D, 0x40, 0x00, 0x00 }, // 'i'
  { 0x40, 0x80, 0x84, 0x7D, 0x00, 0x00 }, // 'j'
  { 0x7F, 0x10, 0x28, 0x44, 0x00, 0x00 }, // 'k'
  { 0x00, 0x41, 0x7F, 0x40, 0x00, 0x00 }, // 'l'
  { 0x7C, 0x04, 0x18, 0x04, 0x78, 0x00 }, // 'm'
  { 0x7C, 0x08, 0x04, 0x04, 0x78, 0x00 }, // 'n'
  { 0x38, 0x44, 0x44, 0x44, 0x38, 0x00 }, // 'o'
  { 0xFC, 0x24, 0x24, 0x24, 0x18, 0x00 }, // 'p'
  { 0x18, 0x24, 0x24, 0x18, 0xFC, 0x00 }, // 'q'
  { 0x7C, 0x08, 0x04, 0x04, 0x08, 0x00 }, // 'r'
  { 0x48, 0x54, 0x54, 0x54, 0x20, 0x00 }, // 's'
  { 0x04, 0x3F, 0x44, 0x40, 0x20, 0x00 }, // 't'
  { 0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00 }, // 'u'
  { 0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00 }, // 'v'
  { 0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00 }, // 'w'
  { 0x44, 0x28, 0x10, 0x28, 0x44, 0x00 }, // 'x'
  { 0x1C, 0xA0, 0xA0, 0xA0, 0x7C, 0x00 }, // 'y'
  { 0x44, 0x64, 0x54, 0x4C, 0x44, 0x00 }, // 'z'
  { 0x00, 0x08, 0x36, 0x41, 0x00, 0x00 }, // '{'
  { 0x00, 0x00, 0x77, 0x00, 0x00, 0x00 }, // '|'
  { 0x00, 0x41, 0x36, 0x08, 0x00, 0x00 }, // '}'
  { 0x08, 0x08, 0x2A, 0x1C, 0x08, 0x00 }  // '~'
};

/* ---------------- SPI 하위 전송 함수 ---------------- */

static void ssd1306SpiWriteCommand(const uint8_t *cmd, uint16_t len)
{
  /* DC = LOW (Command Mode) */
  HAL_GPIO_WritePin(SSD1306_DC_GPIO_Port, SSD1306_DC_Pin, GPIO_PIN_RESET);

  /* CS = LOW (Chip Select) */
  HAL_GPIO_WritePin(SPI1_SS_GPIO_Port, SPI1_SS_Pin, GPIO_PIN_RESET);

  HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, (uint8_t *)cmd, len, 100);

  /* CS = HIGH (Deselect) */
  HAL_GPIO_WritePin(SPI1_SS_GPIO_Port, SPI1_SS_Pin, GPIO_PIN_SET);

  if (status != HAL_OK)
  {
    printf("[SSD1306_SPI] ERR: WriteCommand failed (status=%d)\r\n", status);
  }
}

static void ssd1306SpiWriteData(const uint8_t *data, uint16_t len)
{
  /* DC = HIGH (Data Mode) */
  HAL_GPIO_WritePin(SSD1306_DC_GPIO_Port, SSD1306_DC_Pin, GPIO_PIN_SET);

  /* CS = LOW (Chip Select) */
  HAL_GPIO_WritePin(SPI1_SS_GPIO_Port, SPI1_SS_Pin, GPIO_PIN_RESET);

  HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, (uint8_t *)data, len, 200);

  /* CS = HIGH (Deselect) */
  HAL_GPIO_WritePin(SPI1_SS_GPIO_Port, SPI1_SS_Pin, GPIO_PIN_SET);

  if (status != HAL_OK)
  {
    printf("[SSD1306_SPI] ERR: WriteData failed (status=%d)\r\n", status);
  }
}

/* ---------------- 초기화 및 기본 제어 API ---------------- */

bool ssd1306SpiInit(void)
{
  printf("\r\n========================================\r\n");
  printf("[SSD1306_SPI] Initializing 7-Pin SPI OLED...\r\n");

  /* 1. SPI1 통신 속도 및 데이터 크기 보정
   *    - 기본 CubeMX Prescaler=2(125MHz)는 SSD1306 한계(10MHz)를 초과하므로 32(~7.8MHz)로 낮춤
   *    - DataSize를 8비트로 설정
   */
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32; // 250MHz / 32 = 약 7.8MHz (최적 안정 주파수)
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    printf("[SSD1306_SPI] ERR: HAL_SPI_Init failed!\r\n");
    return false;
  }
  printf("[SSD1306_SPI] SPI1 Configured: 8-Bit, ~7.8MHz (Prescaler 32)\r\n");

  /* 2. 통신 핀 GPIO 속도 고속(High Speed) 재구성 */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

  /* DC Pin (PC13) */
  GPIO_InitStruct.Pin = SSD1306_DC_Pin;
  HAL_GPIO_Init(SSD1306_DC_GPIO_Port, &GPIO_InitStruct);

  /* CS Pin (PA4) */
  GPIO_InitStruct.Pin = SPI1_SS_Pin;
  HAL_GPIO_Init(SPI1_SS_GPIO_Port, &GPIO_InitStruct);

  /* CS 기본 비활성화(HIGH), DC 기본 LOW */
  HAL_GPIO_WritePin(SPI1_SS_GPIO_Port, SPI1_SS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SSD1306_DC_GPIO_Port, SSD1306_DC_Pin, GPIO_PIN_RESET);

  /* 3. 하드웨어 리셋 펄스 수행 (PA10) */
#ifdef SSD1306_RES_Pin
  GPIO_InitStruct.Pin = SSD1306_RES_Pin;
  HAL_GPIO_Init(SSD1306_RES_GPIO_Port, &GPIO_InitStruct);

  printf("[SSD1306_SPI] Performing Hardware Reset on RES Pin...\r\n");
  HAL_GPIO_WritePin(SSD1306_RES_GPIO_Port, SSD1306_RES_Pin, GPIO_PIN_RESET);
  HAL_Delay(20); /* 20ms 리셋 유지 */
  HAL_GPIO_WritePin(SSD1306_RES_GPIO_Port, SSD1306_RES_Pin, GPIO_PIN_SET);
  HAL_Delay(50); /* 전원 및 차지펌프 안정화 대기 (50ms) */
  printf("[SSD1306_SPI] Reset complete. RES pin is HIGH (3.3V)\r\n");
#else
  HAL_Delay(100);
#endif

  /* 4. SSD1306 / SH1106 표준 초기화 커맨드 전송
   *    - 내부 DC-DC 차지펌프 활성화(0x8D 0x14)
   *    - SH1106 호환 DC-DC 활성화(0xAD 0x8B) 동시 포함
   */
  static const uint8_t init_sequence[] = {
    0xAE,             /* Display OFF */
    0xD5, 0x80,       /* Set Display Clock Divide Ratio */
    0xA8, 0x3F,       /* Set Multiplex Ratio (1/64 duty) */
    0xD3, 0x00,       /* Set Display Offset */
    0x40,             /* Set Start Line (line 0) */
    0xAD, 0x8B,       /* SH1106: DC-DC Control Mode ON */
    0x8D, 0x14,       /* SSD1306: Enable Internal Charge Pump (7.5V) */
    0x20, 0x02,       /* Page Addressing Mode */
    0xA1,             /* Segment Re-map (A1: Normal, A0: Inverted) */
    0xC8,             /* COM Output Scan Direction (C8: Normal, C0: Inverted) */
    0xDA, 0x12,       /* Set COM Pins Config (Alternative) */
    0x81, 0xCF,       /* Set Contrast (0xCF: 밝게) */
    0xD9, 0xF1,       /* Set Pre-charge Period */
    0xDB, 0x40,       /* Set VCOMH Deselect Level */
    0xA4,             /* Entire Display ON: Output follows RAM */
    0xA6,             /* Set Normal Display (0xA6: 1=White, 0=Black) */
    0xAF              /* Display ON! */
  };

  printf("[SSD1306_SPI] Sending OLED init sequence (%d bytes)...\r\n", sizeof(init_sequence));
  ssd1306SpiWriteCommand(init_sequence, sizeof(init_sequence));

  HAL_Delay(20);

  /* 화면 전체 클리어 후 GDDRAM 전송 */
  printf("[SSD1306_SPI] Clearing display RAM buffer...\r\n");
  ssd1306SpiClear();
  ssd1306SpiUpdate();

  printf("[SSD1306_SPI] Init Success! Ready to display.\r\n");
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
    /* Page Addressing Mode: 페이지(0~7) 및 컬럼 시작 주소 설정
     * 0x02: SH1106(1.3인치) 132컬럼 패널의 중앙 정렬 오프셋(2픽셀) 호환
     */
    uint8_t page_cmd[3] = {
      (uint8_t)(0xB0 + page), /* Set Page Address (B0 ~ B7) */
      0x00,                   /* Lower Column Address (0x00: 정품 SSD1306 128컬럼 정렬) */
      0x10                    /* Higher Column Address (0x10) */
    };

    ssd1306SpiWriteCommand(page_cmd, 3);
    ssd1306SpiWriteData(&ssd1306_spi_buffer[page * SSD1306_SPI_WIDTH], SSD1306_SPI_WIDTH);
  }
}

/* ---------------- 그래픽스 API ---------------- */

void ssd1306SpiDrawPixel(int16_t x, int16_t y, uint8_t color)
{
  if (x < 0 || x >= SSD1306_SPI_WIDTH || y < 0 || y >= SSD1306_SPI_HEIGHT)
    return;

  if (color == SSD1306_SPI_COLOR_WHITE)
    ssd1306_spi_buffer[x + (y / 8) * SSD1306_SPI_WIDTH] |= (1 << (y % 8));
  else
    ssd1306_spi_buffer[x + (y / 8) * SSD1306_SPI_WIDTH] &= ~(1 << (y % 8));
}

void ssd1306SpiDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
  int16_t dx = abs(x1 - x0);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t dy = -abs(y1 - y0);
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = dx + dy;

  while (1)
  {
    ssd1306SpiDrawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;

    int16_t e2 = 2 * err;
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

void ssd1306SpiDrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
  ssd1306SpiDrawLine(x, y, x + w - 1, y, color);
  ssd1306SpiDrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
  ssd1306SpiDrawLine(x, y, x, y + h - 1, color);
  ssd1306SpiDrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void ssd1306SpiFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
  for (int16_t i = x; i < x + w; i++)
  {
    for (int16_t j = y; j < y + h; j++)
    {
      ssd1306SpiDrawPixel(i, j, color);
    }
  }
}

void ssd1306SpiDrawChar(int16_t x, int16_t y, char ch, uint8_t color)
{
  if (ch < 0x20 || ch > 0x7E)
    ch = ' ';

  const uint8_t *char_data = font6x8[ch - 0x20];

  for (uint8_t col = 0; col < 6; col++)
  {
    uint8_t line = char_data[col];
    for (uint8_t row = 0; row < 8; row++)
    {
      if (line & (1 << row))
        ssd1306SpiDrawPixel(x + col, y + row, color);
      else
        ssd1306SpiDrawPixel(x + col, y + row, !color);
    }
  }
}

void ssd1306SpiDrawString(int16_t x, int16_t y, const char *str, uint8_t color)
{
  int16_t cur_x = x;
  int16_t cur_y = y;

  while (*str)
  {
    if (*str == '\n')
    {
      cur_x = x;
      cur_y += 8;
    }
    else
    {
      if (cur_x + 6 > SSD1306_SPI_WIDTH)
      {
        cur_x = x;
        cur_y += 8;
      }
      if (cur_y + 8 > SSD1306_SPI_HEIGHT)
        break;

      ssd1306SpiDrawChar(cur_x, cur_y, *str, color);
      cur_x += 6;
    }
    str++;
  }
}

void ssd1306SpiTest(void)
{
  printf("[SSD1306_SPI] Rendering Yellow Top Box & Blue Bottom Box...\r\n");
  ssd1306SpiClear();

  /* 1. 상단 노란색 영역 (Y: 0 ~ 15, 총 16라인)
   *    - 상단 전체를 감싸는 노란색 테두리 박스
   *    - 중앙에 노란색 타이틀 텍스트
   */
  ssd1306SpiDrawRect(0, 0, SSD1306_SPI_WIDTH, 15, SSD1306_SPI_COLOR_YELLOW);
  ssd1306SpiDrawString(16, 4, "[ YELLOW BOX ]", SSD1306_SPI_COLOR_YELLOW);

  /* 2. 하단 파란색 영역 (Y: 17 ~ 63, 총 47라인)
   *    - 하단 전체를 감싸는 파란색 테두리 박스
   *    - 내부 구분선 및 파란색 안내 텍스트
   */
  ssd1306SpiDrawRect(0, 17, SSD1306_SPI_WIDTH, 47, SSD1306_SPI_COLOR_BLUE);
  ssd1306SpiDrawString(18, 23, "BLUE BOX DISPLAY", SSD1306_SPI_COLOR_BLUE);
  ssd1306SpiDrawLine(6, 34, 121, 34, SSD1306_SPI_COLOR_BLUE);
  ssd1306SpiDrawString(10, 39, "SPI1: PA5/PA7 7P", SSD1306_SPI_COLOR_BLUE);
  ssd1306SpiDrawString(10, 49, "DC:PC13  CS:PA4", SSD1306_SPI_COLOR_BLUE);

  ssd1306SpiUpdate();
  printf("[SSD1306_SPI] Test Screen Update Completed successfully.\r\n");
}



