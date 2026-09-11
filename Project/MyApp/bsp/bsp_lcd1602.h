#pragma once

#include <stdint.h>
#include <stdbool.h>

#define LCD1602_I2C_ADDR_DEFAULT  0x27
#define LCD1602_I2C_ADDR_ALT      0x3F

/* LCD1602 드라이버 핸들 구조체 (객체 지향 메서드 포함) */
typedef struct {
  void         *dev;            /* 외부에서 주입된 I2C 디바이스 (예: &hi2c1) */
  uint8_t       addr;           /* LCD I2C 슬레이브 주소 (8비트 주소) */
  bool          backlight_state;/* 백라이트 On/Off */
  bool          initialized;    /* 초기화 완료 여부 */

  /* 객체 지향 메서드 */
  void (*sendCommand)(uint8_t cmd);
  void (*sendData)(uint8_t data);
  void (*clear)(void);
  void (*cursor)(uint8_t row, uint8_t col);
  void (*print)(const char *str);
  void (*printf)(const char *fmt, ...);
  void (*backlight)(bool on);
} lcd1602Handle_t;



bool lcd1602Init(lcd1602Handle_t *hlcd, void *dev, uint8_t addr);
void lcd1602SendCommand(lcd1602Handle_t *hlcd, uint8_t cmd);
void lcd1602SendData(lcd1602Handle_t *hlcd, uint8_t data);
void lcd1602Clear(lcd1602Handle_t *hlcd);
void lcd1602Cursor(lcd1602Handle_t *hlcd, uint8_t row, uint8_t col);
void lcd1602Print(lcd1602Handle_t *hlcd, const char *str);
void lcd1602Printf(lcd1602Handle_t *hlcd, const char *fmt, ...);
void lcd1602Backlight(lcd1602Handle_t *hlcd, bool on);


