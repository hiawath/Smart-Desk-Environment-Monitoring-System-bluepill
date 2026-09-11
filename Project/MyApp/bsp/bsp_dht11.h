#pragma once

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* DHT11 핀 구조체 */
typedef struct {
  GPIO_TypeDef *port;
  uint16_t      pin;
} dht11Pin_t;

/* DHT11 측정 데이터 구조체 */
typedef struct {
  float temperature;
  float humidity;
  bool  is_valid;
} dht11Data_t;

/* DHT11 드라이버 핸들 구조체 (객체 지향 메서드 포함) */
typedef struct {
  dht11Pin_t pins;
  float      latest_temperature;
  float      latest_humidity;
  bool       initialized;

  /* 객체 지향 메서드 */
  bool  (*read)(dht11Data_t *data);
  float (*getTemperature)(void);
  float (*getHumidity)(void);
} dht11Handle_t;


void  dht11Init(dht11Handle_t *hdht, const dht11Pin_t *pins);
bool  dht11Read(dht11Handle_t *hdht, dht11Data_t *data);
float dht11GetTemperature(dht11Handle_t *hdht);
float dht11GetHumidity(dht11Handle_t *hdht);
