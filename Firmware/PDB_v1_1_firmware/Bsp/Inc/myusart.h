#ifndef __MYUSART_H
#define __MYUSART_H

#include "main.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stm32f4xx_hal.h"

// USART配置宏
#define RX_BUFFER_SIZE    40 				//USART1接收数据长度
#define RX_BUFFER1_SIZE   16				//处理后的接收数组长度
#define TX_CAN_BUFFER_SIZE   4      //待发送报文

extern int8_t rxBuffer[RX_BUFFER_SIZE];         //接收缓存区
extern uint8_t rxIndex;
extern uint8_t ec_rxBuffer[100];
extern uint8_t ec_rxIndex;

// 函数声明
void USART1_Init(void); // USART1初始化函数
void myprintf(const char *__format, ...);

void convert_to_int8_array(int16_t *input_array, size_t length, int8_t *output_array);
float extract_float_from_buffer(int8_t *buffer, uint16_t start_index);

#endif //__USART_H
