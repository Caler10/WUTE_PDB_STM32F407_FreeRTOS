#ifndef __MYUSART_H
#define __MYUSART_H

#include "main.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stm32f4xx_hal.h"

// USART配置宏

// 放在了.c文件中了

// 函数声明

void USART1_Init(void); // USART1初始化函数
void myprintf(const char *__format, ...);

#endif //__USART_H
