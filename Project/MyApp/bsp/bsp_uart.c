#include "bsp_uart.h"
#include <string.h>
#include <stdio.h>

static uint8_t rx_buf[UART_RX_BUF_SIZE];

/* AC6 세미호스팅 비활성화 (HardFault 방지) */
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm(".global __use_no_semihosting\n\t");

void _sys_exit(int return_code) {
  (void)return_code;
  while (1);
}

void _ttywrch(int ch) {
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
}

#endif

/* printf -> UART2 리디렉션 */
int fputc(int ch, FILE *f) {
  (void)f;
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

/* printf() 출력을 USART2로 리디렉션 */
int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 100);
  return len;
}

void uartInit(void)
{
  /* USART2 DMA IDLE 수신 대기 시작 */
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, UART_RX_BUF_SIZE);
}

/* UART 수신 이벤트 콜백 (IDLE 라인 감지 또는 버퍼 만료) */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART2)
  {
    if (Size > 0)
    {
      /* 에코 전송 */
      HAL_UART_Transmit(&huart2, rx_buf, Size, 100);
      memset(rx_buf, 0, Size);
    }
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, UART_RX_BUF_SIZE);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, UART_RX_BUF_SIZE);
  }
}

