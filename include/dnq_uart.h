#ifndef _DNQ_UART_H_
#define _DNQ_UART_H_

#include "common.h"

#define DNQ_UART_PORT0      "/dev/ttyS0"
#define DNQ_UART_PORT1      "/dev/ttyS1"
#define DNQ_UART_PORT2      "/dev/ttyS2"
#define DNQ_UART_PORT6      "/dev/ttyS6"
#define DNQ_UART_PORT8      "/dev/ttyS8"


#define DNQ_CONSOLE_UART        DNQ_UART_PORT0
#define DNQ_MCU_UART            DNQ_UART_PORT1
#define DNQ_LCD_UART            DNQ_UART_PORT6
#define DNQ_SENSOR_UART         DNQ_UART_PORT8

S32 dnq_uart_init();
S32 dnq_uart_deinit();
S32 dnq_uart_set_baudrate(S32 fd, U32 baudrate);
S32 dnq_uart_set_timeout(S32 fd, U32 timeout, U32 read_min);
S32 dnq_lcd_uart_read(U8 *buffer, U32 len);
S32 dnq_lcd_uart_write(U8 *buffer, U32 len);
S32 dnq_mcu_uart_read(U8 *buffer, U32 len);
S32 dnq_mcu_uart_write(U8 *buffer, U32 len);
S32 dnq_sensor_uart_read(U8 *buffer, U32 len);
S32 dnq_sensor_uart_write(U8 *buffer, U32 len);
S32 dnq_sensor_uart_sync();


#endif /* _DNQ_UART_H_ */

