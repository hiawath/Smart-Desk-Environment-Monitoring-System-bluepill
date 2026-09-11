#include "bsp_uart.h"
#include <string.h>
#include <stdio.h>
static uint8_t rx_data;
static uint8_t rx_buf[UART_RX_BUF_SIZE];

/* AC6 세미호스팅 비활성화 (HardFault 방지) */
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm(".global __use_no_semihosting\n\t");

void _sys_exit(int return_code) {
  while (1);
}

void _ttywrch(int ch) {
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
}

#endif
/* printf -> UART2 리디렉션 */
int fputc(int ch, FILE *f) {
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}


/* printf() 출력을 USART2로 리디렉션 */
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 100);
  return len;
}


void uartInit(void)
{
  /* USART2 DMA 수신 대기 시작 */
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, UART_RX_BUF_SIZE);
}

/* UART 수신 콜백 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    if (rx_data == 'a')
      printf("Hello STM32 Cortex-M4 USART Polling!\r\n");
    else
      HAL_UART_Transmit(&huart2, rx_buf, UART_RX_BUF_SIZE, 100);

    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, UART_RX_BUF_SIZE);
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART2)
  {
    if (rx_data == 'a')
      printf("Hello STM32 Cortex-M4 USART Polling!\r\n");
    else
      HAL_UART_Transmit(&huart2, rx_buf, Size, 100);

    HAL_UART_DMAStop(&huart2);
    memset(rx_buf, 0, Size);
  }
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, UART_RX_BUF_SIZE);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  HAL_UART_Receive_DMA(&huart2, rx_buf, UART_RX_BUF_SIZE);
}
