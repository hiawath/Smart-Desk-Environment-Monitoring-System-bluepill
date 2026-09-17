#include "appInit.h"
#include "bsp/bsp_ssd1306_spi.h"
#include "i2c.h"

/* DS1302 드라이버 핸들 */
ds1302Handle_t hds1302;
/* DHT11 드라이버 핸들 */
dht11Handle_t hdht11;
/* LCD1602 드라이버 핸들 */
lcd1602Handle_t hlcd1602;
/* HC-SR04 초음파 센서 드라이버 핸들 */
hcSr04Handle_t  hhcSr04;

void appInit(void)
{
  /* 하드웨어 레이어 초기화 (GPIO, UART, I2C, SSD1306, LCD1602 등) */
  bspInit();

  /* 보드 부팅 및 I2C 버스 안정화를 위해 100ms 대기 */
  HAL_Delay(100);

  /* I2C1 버스 스캔 실행 */
  //i2cScan();

  /* 스캔 후 I2C 버스 정리 및 SSD1306 초기화 확인 */
  //i2cBusRecover();
  HAL_Delay(10);
}

void bspInit(void)
{
  delayInit();
  gpioInit();
  uartInit();
  i2cInit();

  /* LCD1602: I2C1 디바이스 인스턴스 및 슬레이브 주소(0x27) 주입 */
  lcd1602Init(&hlcd1602, &hi2c1, LCD1602_I2C_ADDR_DEFAULT);

  if (hssd1306Spi.init()) {
    hssd1306Spi.clear();
  }
  


  adcInit();

  /* DS1302: CubeMX Label 매크로로 핀 정보 주입 */
  ds1302Pin_t ds1302_pins = {
    .rst_port = DS1302_RST_GPIO_Port,
    .rst_pin  = DS1302_RST_Pin,
    .dat_port = DS1302_DATA_GPIO_Port,
    .dat_pin  = DS1302_DATA_Pin,
    .clk_port = DS1302_CLK_GPIO_Port,
    .clk_pin  = DS1302_CLK_Pin,
  };
  ds1302Init(&hds1302, &ds1302_pins);

  /* DHT11: CubeMX Label 매크로로 핀 정보 주입 */
  dht11Pin_t dht11_pins = {
    .port = DHT11_GPIO_Port,
    .pin  = DHT11_Pin,
  };
  dht11Init(&hdht11, &dht11_pins);

  /* HC-SR04: CubeMX Label 매크로로 핀 정보 주입 */
  hcSr04Pin_t hcsr04_pins = {
    .trig_port = HCSR04_TRIG_GPIO_Port,
    .trig_pin  = HCSR04_TRIG_Pin,
    .echo_port = HCSR04_ECHO_GPIO_Port,
    .echo_pin  = HCSR04_ECHO_Pin,
  };
  hcSr04Init(&hhcSr04, &hcsr04_pins);

  timerInit();

}


