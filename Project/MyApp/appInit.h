#pragma once

#include "main.h"

#include "bsp_adc.h"
#include "bsp_delay.h"
#include "bsp_dht11.h"
#include "bsp_ds1302.h"
#include "bsp_gpio.h"
#include "bsp_hcsr04.h"
#include "bsp_i2c.h"
#include "bsp_lcd1602.h"
#include "bsp_ssd1306.h"
#include "bsp_uart.h"
#include "bsp_timer.h"
#include "bsp_ssd1306_spi.h"


/* DS1302 드라이버 핸들 */
extern ds1302Handle_t hds1302;
/* DHT11 드라이버 핸들 */
extern dht11Handle_t hdht11;
/* LCD1602 드라이버 핸들 */
extern lcd1602Handle_t hlcd1602;
/* HC-SR04 초음파 센서 드라이버 핸들 */
extern hcSr04Handle_t  hhcSr04;

void appInit(void);
void bspInit(void);


