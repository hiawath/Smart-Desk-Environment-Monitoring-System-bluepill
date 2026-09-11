#include "bsp_lcd1602.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>

/* ----------------------------------------------------------------
 * 내부 헬퍼 함수들
 * ----------------------------------------------------------------*/

static bool i2c_send(lcd1602Handle_t *hlcd, uint8_t *buf, uint16_t len)
{
  if (!hlcd || !hlcd->dev) return false;

  I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)hlcd->dev;
  HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, hlcd->addr, buf, len, 20);
  if (ret != HAL_OK)
  {
    /* 일시적 BUSY/NACK 시 짧은 딜레이 후 1회 재시도 */
    HAL_Delay(1);
    ret = HAL_I2C_Master_Transmit(hi2c, hlcd->addr, buf, len, 20);
  }
  return (ret == HAL_OK);
}

static bool lcd1602_is_device_ready(lcd1602Handle_t *hlcd, uint8_t addr)
{
  if (!hlcd || !hlcd->dev) return false;
  I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)hlcd->dev;
  return (HAL_I2C_IsDeviceReady(hi2c, addr, 2, 10) == HAL_OK);
}

/**
 * @brief  HD44780: 4비트 nibble 전송 + Enable 스트로브
 *         4바이트를 한 번에 전송하여 I2C 트랜잭션을 최소화
 */
static void lcd1602_write(lcd1602Handle_t *hlcd, uint8_t data, uint8_t rs)
{
  if (!hlcd || !hlcd->initialized) return;

  uint8_t bl   = hlcd->backlight_state ? 0x08 : 0x00;
  uint8_t high = data & 0xF0;
  uint8_t low  = (data << 4) & 0xF0;

  uint8_t buf[4];
  buf[0] = high | bl | rs | 0x04; // High nibble EN=1
  buf[1] = high | bl | rs;        // High nibble EN=0
  buf[2] = low  | bl | rs | 0x04; // Low  nibble EN=1
  buf[3] = low  | bl | rs;        // Low  nibble EN=0

  if (!i2c_send(hlcd, buf, 4))
  {
    hlcd->initialized = false; // 재시도 후도 실패 → 이후 호출 비활성화
  }
}

static lcd1602Handle_t *s_active_lcd = NULL;

/* ----------------------------------------------------------------
 * 객체 지향 메서드 래퍼 함수들 (lcd1602_ 네임스페이스 접두사 적용)
 * ----------------------------------------------------------------*/

static void lcd1602_sendCommand(uint8_t cmd)
{
  lcd1602SendCommand(s_active_lcd, cmd);
}

static void lcd1602_sendData(uint8_t data)
{
  lcd1602SendData(s_active_lcd, data);
}

static void lcd1602_clear(void)
{
  lcd1602Clear(s_active_lcd);
}

static void lcd1602_cursor(uint8_t row, uint8_t col)
{
  lcd1602Cursor(s_active_lcd, row, col);
}

static void lcd1602_print(const char *str)
{
  lcd1602Print(s_active_lcd, str);
}

static void lcd1602_printf(const char *fmt, ...)
{
  if (!s_active_lcd || !fmt) return;
  char buf[33];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  lcd1602Print(s_active_lcd, buf);
}

static void lcd1602_backlight(bool on)
{
  lcd1602Backlight(s_active_lcd, on);
}

/* ----------------------------------------------------------------
 * 공개 API
 * ----------------------------------------------------------------*/

bool lcd1602Init(lcd1602Handle_t *hlcd, void *dev, uint8_t addr)
{
  if (!hlcd || !dev)
  {
    return false;
  }

  hlcd->dev             = dev;
  hlcd->backlight_state = true;
  hlcd->initialized     = false;

  s_active_lcd          = hlcd;

  /* 메서드 함수 포인터 바인딩 */
  hlcd->sendCommand        = lcd1602_sendCommand;
  hlcd->sendData           = lcd1602_sendData;
  hlcd->clear              = lcd1602_clear;
  hlcd->cursor             = lcd1602_cursor;
  hlcd->print              = lcd1602_print;
  hlcd->printf             = lcd1602_printf;
  hlcd->backlight          = lcd1602_backlight;

  HAL_Delay(50); // 전원 인가 후 대기 (>40ms)

  /* 외부에서 주소가 주입된 경우 (addr != 0) */
  if (addr != 0)
  {
    /* 7비트 주소(0x01~0x7F)로 입력된 경우 8비트 주소로 좌측 1비트 시프트 */
    hlcd->addr = (addr < 0x80) ? (addr << 1) : addr;

    if (!lcd1602_is_device_ready(hlcd, hlcd->addr))
    {
      return false;
    }
  }
  else
  {
    /* addr가 0이면 자동 감지: 0x27 우선, 없으면 0x3F */
    if (lcd1602_is_device_ready(hlcd, 0x27 << 1))
    {
      hlcd->addr = 0x27 << 1;
    }
    else if (lcd1602_is_device_ready(hlcd, 0x3F << 1))
    {
      hlcd->addr = 0x3F << 1;
    }
    else
    {
      return false;
    }
  }

  hlcd->initialized = true;

  uint8_t bl = 0x08;
  uint8_t cmd2[2];

  /* HD44780 초기화 시퀀스 (8비트 → 4비트 전환) */
  cmd2[0] = 0x30 | bl | 0x04;
  cmd2[1] = 0x30 | bl;
  i2c_send(hlcd, cmd2, 2);
  HAL_Delay(5);

  i2c_send(hlcd, cmd2, 2);
  HAL_Delay(1);

  i2c_send(hlcd, cmd2, 2);
  HAL_Delay(1);

  cmd2[0] = 0x20 | bl | 0x04; // 4비트 모드 전환
  cmd2[1] = 0x20 | bl;
  i2c_send(hlcd, cmd2, 2);
  HAL_Delay(1);

  /* 기능 설정 */
  lcd1602SendCommand(hlcd, 0x28); // 4-bit, 2라인, 5x8
  lcd1602SendCommand(hlcd, 0x0C); // Display ON, Cursor OFF
  lcd1602SendCommand(hlcd, 0x06); // Entry mode: 커서 우향
  lcd1602Clear(hlcd);

  return hlcd->initialized;
}

void lcd1602SendCommand(lcd1602Handle_t *hlcd, uint8_t cmd)
{
  if (!hlcd) return;
  lcd1602_write(hlcd, cmd, 0x00); // RS=0
  if (cmd == 0x01 || cmd == 0x02)
  {
    HAL_Delay(2); // Clear/Home은 >1.52ms 소요
  }
}

void lcd1602SendData(lcd1602Handle_t *hlcd, uint8_t data)
{
  if (!hlcd) return;
  lcd1602_write(hlcd, data, 0x01); // RS=1
}

void lcd1602Clear(lcd1602Handle_t *hlcd)
{
  lcd1602SendCommand(hlcd, 0x01);
}

void lcd1602Cursor(lcd1602Handle_t *hlcd, uint8_t row, uint8_t col)
{
  uint8_t addr = (row == 0) ? (0x00 + col) : (0x40 + col);
  lcd1602SendCommand(hlcd, 0x80 | addr);
}

void lcd1602Print(lcd1602Handle_t *hlcd, const char *str)
{
  if (!hlcd || !str) return;
  while (*str)
  {
    lcd1602SendData(hlcd, (uint8_t)(*str++));
  }
}

void lcd1602Printf(lcd1602Handle_t *hlcd, const char *fmt, ...)
{
  if (!hlcd || !fmt) return;
  char buf[33];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  lcd1602Print(hlcd, buf);
}

void lcd1602Backlight(lcd1602Handle_t *hlcd, bool on)
{
  if (!hlcd || !hlcd->initialized) return;
  hlcd->backlight_state = on;
  uint8_t val = hlcd->backlight_state ? 0x08 : 0x00;
  i2c_send(hlcd, &val, 1);
}


