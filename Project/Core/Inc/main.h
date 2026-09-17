/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LD2_Pin GPIO_PIN_13
#define LD2_GPIO_Port GPIOC
#define BGR_GND_Pin GPIO_PIN_0
#define BGR_GND_GPIO_Port GPIOB
#define BGR_R_Pin GPIO_PIN_1
#define BGR_R_GPIO_Port GPIOB
#define BGR_G_Pin GPIO_PIN_10
#define BGR_G_GPIO_Port GPIOB
#define BGR_B_Pin GPIO_PIN_11
#define BGR_B_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* DS1302 Real-Time Clock Pins */
#define DS1302_RST_Pin          GPIO_PIN_12
#define DS1302_RST_GPIO_Port    GPIOB
#define DS1302_DATA_Pin         GPIO_PIN_13
#define DS1302_DATA_GPIO_Port   GPIOB
#define DS1302_CLK_Pin          GPIO_PIN_14
#define DS1302_CLK_GPIO_Port    GPIOB

/* DHT11 Temperature & Humidity Sensor Pin */
#define DHT11_Pin               GPIO_PIN_15
#define DHT11_GPIO_Port         GPIOB

/* HC-SR04 Ultrasonic Distance Sensor Pins */
#define HCSR04_TRIG_Pin         GPIO_PIN_8
#define HCSR04_TRIG_GPIO_Port   GPIOA
#define HCSR04_ECHO_Pin         GPIO_PIN_9
#define HCSR04_ECHO_GPIO_Port   GPIOA

/* SSD1306 SPI OLED Display Pins */
#define SSD1306_CS_Pin          GPIO_PIN_4
#define SSD1306_CS_GPIO_Port    GPIOA
#define SSD1306_SCK_Pin         GPIO_PIN_5
#define SSD1306_SCK_GPIO_Port   GPIOA
#define SSD1306_DC_Pin          GPIO_PIN_6
#define SSD1306_DC_GPIO_Port    GPIOA
#define SSD1306_MOSI_Pin        GPIO_PIN_7
#define SSD1306_MOSI_GPIO_Port  GPIOA
#define SSD1306_RES_Pin         GPIO_PIN_1
#define SSD1306_RES_GPIO_Port   GPIOA

/* Legacy alias for SPI1_SS_Pin */
#define SPI1_SS_Pin             SSD1306_CS_Pin
#define SPI1_SS_GPIO_Port       SSD1306_CS_GPIO_Port

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
