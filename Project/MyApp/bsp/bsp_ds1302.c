#include "bsp_ds1302.h"
#include "bsp_delay.h"
#include <string.h>
#include <stdio.h>

/* DS1302 레지스터 주소 */
#define DS1302_REG_SEC           0x80
#define DS1302_REG_MIN           0x82
#define DS1302_REG_HOUR          0x84
#define DS1302_REG_DATE          0x86
#define DS1302_REG_MONTH         0x88
#define DS1302_REG_DAY           0x8A
#define DS1302_REG_YEAR          0x8C
#define DS1302_REG_WP            0x8E
#define DS1302_REG_TRICKLE       0x90
#define DS1302_REG_BURST_CLOCK   0xBE

/* GPIO 제어 매크로 (핸들 기반) */
#define RST_HIGH(h)  HAL_GPIO_WritePin((h)->pins.rst_port, (h)->pins.rst_pin, GPIO_PIN_SET)
#define RST_LOW(h)   HAL_GPIO_WritePin((h)->pins.rst_port, (h)->pins.rst_pin, GPIO_PIN_RESET)
#define CLK_HIGH(h)  HAL_GPIO_WritePin((h)->pins.clk_port, (h)->pins.clk_pin, GPIO_PIN_SET)
#define CLK_LOW(h)   HAL_GPIO_WritePin((h)->pins.clk_port, (h)->pins.clk_pin, GPIO_PIN_RESET)
#define DAT_HIGH(h)  HAL_GPIO_WritePin((h)->pins.dat_port, (h)->pins.dat_pin, GPIO_PIN_SET)
#define DAT_LOW(h)   HAL_GPIO_WritePin((h)->pins.dat_port, (h)->pins.dat_pin, GPIO_PIN_RESET)
#define DAT_READ(h)  HAL_GPIO_ReadPin((h)->pins.dat_port, (h)->pins.dat_pin)

static inline uint8_t decToBcd(uint8_t val)
{
  return (uint8_t)(((val / 10) << 4) | (val % 10));
}

static inline uint8_t bcdToDec(uint8_t val)
{
  return (uint8_t)(((val >> 4) * 10) + (val & 0x0F));
}

/* DAT 핀을 Output Push-Pull 모드로 전환 (MCU -> DS1302 쓰기용) */
static void ds1302SetDatOutput(ds1302Handle_t *hds)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin   = hds->pins.dat_pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(hds->pins.dat_port, &GPIO_InitStruct);
}

/* DAT 핀을 Input Pull-up 모드로 전환 (DS1302 -> MCU 읽기용) */
static void ds1302SetDatInput(ds1302Handle_t *hds)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin  = hds->pins.dat_pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(hds->pins.dat_port, &GPIO_InitStruct);
}

static void ds1302GpioInit(ds1302Handle_t *hds)
{
  /* GPIOB 클럭 활성화 보장 */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* RST, CLK: Output Push-Pull */
  if (hds->pins.rst_port == hds->pins.clk_port)
  {
    GPIO_InitStruct.Pin   = hds->pins.rst_pin | hds->pins.clk_pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(hds->pins.rst_port, &GPIO_InitStruct);
  }
  else
  {
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    GPIO_InitStruct.Pin = hds->pins.rst_pin;
    HAL_GPIO_Init(hds->pins.rst_port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = hds->pins.clk_pin;
    HAL_GPIO_Init(hds->pins.clk_port, &GPIO_InitStruct);
  }

  /* DAT: 초기 기본 Output Push-Pull 모드 */
  ds1302SetDatOutput(hds);

  /* 초기 핀 상태: CE(RST)=LOW, CLK=LOW, DAT=LOW */
  RST_LOW(hds);
  CLK_LOW(hds);
  DAT_LOW(hds);
}

static void ds1302WriteByte(ds1302Handle_t *hds, uint8_t data)
{
  ds1302SetDatOutput(hds);

  for (uint8_t i = 0; i < 8; i++)
  {
    if (data & 0x01)
      DAT_HIGH(hds);
    else
      DAT_LOW(hds);

    delayUs(2);

    CLK_HIGH(hds);
    delayUs(2);
    CLK_LOW(hds);
    delayUs(2);

    data >>= 1;
  }
}

/* 순수 8비트 데이터 수신 (호출 전 DAT 핀이 Input 모드여야 함) */
static uint8_t ds1302ReadByte(ds1302Handle_t *hds)
{
  uint8_t data = 0;

  for (uint8_t i = 0; i < 8; i++)
  {
    if (DAT_READ(hds) == GPIO_PIN_SET)
    {
      data |= (1 << i);
    }
    CLK_HIGH(hds);
    delayUs(2);
    CLK_LOW(hds);
    delayUs(2);
  }

  return data;
}

static void ds1302WriteReg(ds1302Handle_t *hds, uint8_t reg, uint8_t value)
{
  RST_LOW(hds);
  CLK_LOW(hds);
  delayUs(4);

  RST_HIGH(hds);
  delayUs(4);

  ds1302WriteByte(hds, reg & 0xFE); /* Write Command (Bit 0 = 0) */
  ds1302WriteByte(hds, value);

  delayUs(2);
  RST_LOW(hds);
  delayUs(4);
}

static uint8_t ds1302ReadReg(ds1302Handle_t *hds, uint8_t reg)
{
  uint8_t val = 0;

  RST_LOW(hds);
  CLK_LOW(hds);
  delayUs(4);

  RST_HIGH(hds);
  delayUs(4);

  ds1302WriteByte(hds, reg | 0x01); /* Read Command (Bit 0 = 1) */

  /* 읽기 전 DAT 핀을 Input Pull-up 모드로 1회 전환 */
  ds1302SetDatInput(hds);
  delayUs(2);

  val = ds1302ReadByte(hds);

  delayUs(2);
  RST_LOW(hds);
  delayUs(4);

  /* 읽기 종료 후 다음 쓰기를 위해 DAT를 Output으로 복귀 */
  ds1302SetDatOutput(hds);

  return val;
}

static void ds1302ReadBurstClock(ds1302Handle_t *hds, uint8_t *buf)
{
  RST_LOW(hds);
  CLK_LOW(hds);
  delayUs(4);

  RST_HIGH(hds);
  delayUs(4);

  ds1302WriteByte(hds, DS1302_REG_BURST_CLOCK | 0x01); /* 0xBF: Clock Burst Read */

  /* 8바이트 전체 수신 동안 DAT 핀을 Input Pull-up 모드로 단 1회 유지 (버스 충돌 방지) */
  ds1302SetDatInput(hds);
  delayUs(2);

  for (uint8_t i = 0; i < 8; i++)
  {
    buf[i] = ds1302ReadByte(hds);
  }

  delayUs(2);
  RST_LOW(hds);
  delayUs(4);

  /* 버스트 읽기 완료 후 다음 쓰기를 위해 DAT를 Output으로 복귀 */
  ds1302SetDatOutput(hds);
}

static void ds1302WriteBurstClock(ds1302Handle_t *hds, const uint8_t *buf)
{
  ds1302WriteReg(hds, DS1302_REG_WP, 0x00); /* Write Protect 해제 */

  RST_LOW(hds);
  CLK_LOW(hds);
  delayUs(4);

  RST_HIGH(hds);
  delayUs(4);

  ds1302WriteByte(hds, DS1302_REG_BURST_CLOCK & 0xFE); /* 0xBE: Clock Burst Write */

  for (uint8_t i = 0; i < 8; i++)
  {
    ds1302WriteByte(hds, buf[i]);
  }

  delayUs(2);
  RST_LOW(hds);
  delayUs(4);

  ds1302WriteReg(hds, DS1302_REG_WP, 0x80); /* Write Protect 활성화 */
}

static const char* const day_names[] = {
  "ERR", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

const char* ds1302GetDayStr(uint8_t day_of_week)
{
  if (day_of_week >= 1 && day_of_week <= 7)
  {
    return day_names[day_of_week];
  }
  return day_names[0];
}

static uint8_t calculateDayOfWeek(uint16_t y, uint8_t m, uint8_t d)
{
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  /* 0 = Sunday, 1 = Monday, ..., 6 = Saturday */
  int dow = (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
  return (uint8_t)(dow + 1); /* DS1302: 1 = Sunday, 2 = Monday, ..., 7 = Saturday */
}

void ds1302SetDateTime(ds1302Handle_t *hds, const ds1302Time_t *time)
{
  if (!hds || !time)
    return;

  uint8_t buf[8];
  buf[0] = decToBcd(time->sec)  & 0x7F; /* CH=0 (발진기 가동) */
  buf[1] = decToBcd(time->min)  & 0x7F;
  buf[2] = decToBcd(time->hour) & 0x3F; /* 24시간 모드 (Bit 7 = 0) */
  buf[3] = decToBcd(time->day)  & 0x3F;
  buf[4] = decToBcd(time->month)       & 0x1F;
  buf[5] = decToBcd(time->day_of_week) & 0x07;
  buf[6] = decToBcd((uint8_t)(time->year % 100));
  buf[7] = 0x80; /* WP Enable */

  ds1302WriteBurstClock(hds, buf);
}

void ds1302SetTime(ds1302Handle_t *hds, uint16_t year, uint8_t month, uint8_t day,
                   uint8_t hour, uint8_t min, uint8_t sec)
{
  ds1302Time_t t;
  t.year        = year;
  t.month       = month;
  t.day         = day;
  t.day_of_week = calculateDayOfWeek(year, month, day);
  t.hour        = hour;
  t.min         = min;
  t.sec         = sec;

  ds1302SetDateTime(hds, &t);
}

bool ds1302GetDateTime(ds1302Handle_t *hds, ds1302Time_t *time)
{
  if (!hds || !time || !hds->initialized)
    return false;

  uint8_t buf[8] = {0};
  ds1302ReadBurstClock(hds, buf);

  uint8_t sec_raw  = buf[0];
  uint8_t min_raw  = buf[1];
  uint8_t hour_raw = buf[2];
  uint8_t date_raw = buf[3];
  uint8_t mon_raw  = buf[4];
  uint8_t day_raw  = buf[5];
  uint8_t year_raw = buf[6];

  /* 만약 시계가 멈춘 상태(CH=1)라면 빌드 타임으로 1회 자가 복구 시도 */
  if (sec_raw & 0x80)
  {
    ds1302SetBuildTime(hds);
    ds1302ReadBurstClock(hds, buf);
    sec_raw = buf[0];
    if (sec_raw & 0x80)
    {
      return false;
    }
    min_raw  = buf[1];
    hour_raw = buf[2];
    date_raw = buf[3];
    mon_raw  = buf[4];
    day_raw  = buf[5];
    year_raw = buf[6];
  }

  time->sec         = bcdToDec(sec_raw & 0x7F);
  time->min         = bcdToDec(min_raw & 0x7F);

  /* Hour 레지스터 디코딩 (12시간 / 24시간 모드 완벽 지원) */
  if (hour_raw & 0x80)
  {
    /* 12시간 모드 (Bit 7 = 1): Bit 5는 AM/PM (1 = PM), Bit 4..0은 BCD Hour */
    bool is_pm = (hour_raw & 0x20) != 0;
    uint8_t h = bcdToDec(hour_raw & 0x1F);
    if (is_pm && h < 12) h += 12;
    if (!is_pm && h == 12) h = 0;
    time->hour = h;
  }
  else
  {
    /* 24시간 모드 (Bit 7 = 0): Bit 5..4는 10-Hour, Bit 3..0은 Unit-Hour */
    time->hour = bcdToDec(hour_raw & 0x3F);
  }

  time->day         = bcdToDec(date_raw & 0x3F);
  time->month       = bcdToDec(mon_raw & 0x1F);
  time->day_of_week = bcdToDec(day_raw & 0x07);
  time->year        = 2000 + bcdToDec(year_raw);

  /* 시간 데이터 유효성 검증 */
  if (time->hour > 23 || time->min > 59 || time->sec > 59 ||
      time->month == 0 || time->month > 12 || time->day == 0 || time->day > 31)
  {
    return false;
  }

  return true;
}

/**
  * @brief  빌드 날짜와 시간(__DATE__, __TIME__)으로 DS1302 시간 설정
  *         sscanf 미사용 — 스택 절약을 위해 수동 파싱 사용
  */
void ds1302SetBuildTime(ds1302Handle_t *hds)
{
  const char *time_str = __TIME__; /* "hh:mm:ss" */
  const char *date_str = __DATE__; /* "Mmm dd yyyy" */

  /* month 문자열 파싱 (앞 3글자) */
  char month_str[4] = { date_str[0], date_str[1], date_str[2], '\0' };

  /* day 파싱 (' ' 공백 처리 포함: " 5" vs "15") */
  int day  = (date_str[4] == ' ' ? 0 : (date_str[4] - '0') * 10) + (date_str[5] - '0');

  /* year 파싱 */
  int year = (date_str[7] - '0') * 1000 + (date_str[8] - '0') * 100 +
             (date_str[9] - '0') * 10   + (date_str[10] - '0');

  /* time 파싱 "hh:mm:ss" */
  int hour = (time_str[0] - '0') * 10 + (time_str[1] - '0');
  int min  = (time_str[3] - '0') * 10 + (time_str[4] - '0');
  int sec  = (time_str[6] - '0') * 10 + (time_str[7] - '0');

  static const char *months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  uint8_t month = 1;
  for (uint8_t i = 0; i < 12; i++)
  {
    if (strncmp(month_str, months[i], 3) == 0)
    {
      month = i + 1;
      break;
    }
  }

  ds1302SetTime(hds, (uint16_t)year, month, (uint8_t)day,
                (uint8_t)hour, (uint8_t)min, (uint8_t)sec);
}

void ds1302Init(ds1302Handle_t *hds, const ds1302Pin_t *pins)
{
  if (!hds || !pins)
    return;

  hds->pins        = *pins;
  hds->initialized = false;

  ds1302GpioInit(hds);
  hds->initialized = true;

  printf("\r\n========================================\r\n");
  printf("[DS1302] Initializing RTC (PB12:RST, PB13:DAT, PB14:CLK)...\r\n");

  /* Write Protect 해제 */
  ds1302WriteReg(hds, DS1302_REG_WP, 0x00);

  /* Clock Halt(CH) 확인 */
  uint8_t sec = ds1302ReadReg(hds, DS1302_REG_SEC);
  printf("[DS1302] Initial SEC read: 0x%02X (CH bit: %d)\r\n", sec, (sec & 0x80) ? 1 : 0);

  /*
   * RTC 시간 유효성 검사 및 동기화:
   * 1. CH(Clock Halt) 비트가 1이거나(시계 정지 상태)
   * 2. 기존 레지스터 시간이 비정상(34시 등 hour > 23, year < 2025 등)인 경우
   * 3. 현재 빌드 타임으로 RTC 갱신
   */
  ds1302Time_t current_time;
  bool is_valid = ds1302GetDateTime(hds, &current_time);

  /* 사용자가 요청한 현재 시간 동기화를 위해 현재 빌드 타임으로 설정 */
  printf("[DS1302] Synchronizing to current build time: %s %s\r\n", __DATE__, __TIME__);
  ds1302SetBuildTime(hds);

  if (ds1302GetDateTime(hds, &current_time))
  {
    printf("[DS1302] RTC OK! Current Time: %04d-%02d-%02d %02d:%02d:%02d (%s)\r\n",
           current_time.year, current_time.month, current_time.day,
           current_time.hour, current_time.min, current_time.sec,
           ds1302GetDayStr(current_time.day_of_week));
  }
  else
  {
    printf("[DS1302] Warning: Failed to read RTC time after sync!\r\n");
  }
  printf("========================================\r\n\r\n");
}
