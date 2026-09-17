# Smart Desk Environment Monitoring System (Phase 1)

## Project Overview
This project converts an STM32F103 (Blue Pill) into a comprehensive desktop monitoring tool that tracks time, temperature, humidity, and distance using various sensors.

## Features (Phase 1)
- **Time Keeping:** Real-time clock using DS1302.
- **Environment Sensing:** Temperature and Humidity monitoring via DHT11.
- **Proximity Sensing:** Distance measurement with HC-SR04.
- **Dual Display:**
    - SSD1306 OLED (SPI): Visual UI with Bar Gauges and Spinners.
    - LCD1602 (I2C): Detailed text-based data display.
- **Smart Power:** Logic for auto-screen on/off based on user proximity.

## Hardware Components
- MCU: STM32F103C8T6
- Sensors: DHT11, HC-SR04, DS1302, Internal ADC
- Displays: SSD1306 (128x64 SPI), LCD1602 (I2C)

## Software Architecture
- Non-blocking task scheduler using `HAL_GetTick()`.
- Periodic tasks: 100ms, 500ms, 1s, 2s.
- EMA (Exponential Moving Average) filter for smooth distance readings.
