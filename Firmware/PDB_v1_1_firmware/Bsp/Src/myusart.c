// 置1使能该驱动库
#if 1

#include "mycan.h" //串口转CAN需要
#include "usart.h"
#include "myusart.h"
// freertos
#include "cmsis_os.h" //已引用FreeRTOS.h
#include "queue.h"
#include "stream_buffer.h"
#include "event_groups.h"
#include "semphr.h"

extern EventGroupHandle_t UsartReceiveEventHandle;

uint8_t ec_rxBuffer[100];
uint8_t ec_rxIndex;
uint8_t commandSuccess;

#define IMU_ID 0x0F146D72 // 宏定义惯导的ID编号

#define TX_BUF_LEN 256 /* 发送缓冲区容量，根据需要进行调整 */ // 原256，因为无线透传例程只要64就够了，so
uint8_t TxBuf[TX_BUF_LEN];                                    // 发送缓冲区

int8_t rxBuffer[RX_BUFFER_SIZE]; // 接收缓存区
uint8_t rxIndex = 0;

int8_t rxBuffer1[RX_BUFFER1_SIZE]; // 接受处理缓存区1
uint8_t rxIndex1 = 0;

// int8_t rxBuffer2[16];                   //接收处理缓存区2

int16_t txcanBuffer[TX_CAN_BUFFER_SIZE]; // 待发送数据区
int8_t txcanBuffer1[8];
uint8_t txcanIndex = 0;

// 方案一：自己的打印函数 不基于printf函数
// 还是会进半主机？因为用了printf残留，因调用了“stdio.h”所以编译不会报错，但未重定向未禁止半主机，进半主机，还串口打印不出来，不清楚情况
void myprintf(const char *__format, ...)
{
  va_list ap;
  va_start(ap, __format);

  /* 清空发送缓冲区 */
  memset(TxBuf, 0x0, TX_BUF_LEN);

  /* 填充发送缓冲区 */
  vsnprintf((char *)TxBuf, TX_BUF_LEN, (const char *)__format, ap);
  va_end(ap);
  int len = strlen((const char *)TxBuf);

  /* 往串口发送数据 */
  HAL_UART_Transmit(&huart1, (uint8_t *)&TxBuf, len, 0xFFFF);
}

// 方案二、三：基于printf函数 1.重定向 2.使用microLIB库或者禁止半主机状态（文档里有如何操作，但还未成功（编译器6），ATKESP8266串口透传使用方案2用编译器6可以成功）

// 方案二：禁止半主机
/* 加入以下代码, 支持printf函数, 而不需要选择use MicroLIB */

// #if 1
// #if (__ARMCC_VERSION >= 6010050)            /* 使用AC6编译器时 */
//__asm(".global __use_no_semihosting\n\t");  /* 声明不使用半主机模式 */
//__asm(".global __ARM_use_no_argv \n\t");    /* AC6下需要声明main函数为无参数格式，否则部分例程可能出现半主机模式 */
//
// #else
///* 使用AC5编译器时, 要在这里定义__FILE 和 不使用半主机模式 */
// #pragma import(__use_no_semihosting)
//
// struct __FILE
//{
//     int handle;
//     /* Whatever you require here. If the only file you are using is */
//     /* standard output using printf() for debugging, no file handling */
//     /* is required. */
// };
//
// #endif
//
///* 不使用半主机模式，至少需要重定义_ttywrch\_sys_exit\_sys_command_string函数,以同时兼容AC6和AC5模式 */
// int _ttywrch(int ch)
//{
//     ch = ch;
//     return ch;
// }
//
///* 定义_sys_exit()以避免使用半主机模式 */
// void _sys_exit(int x)
//{
//     x = x;
// }
//
// char *_sys_command_string(char *cmd, int len)
//{
//     return NULL;
// }
//
///* FILE 在 stdio.h里面定义. */
// FILE __stdout;
//
///* 重定义fputc函数, printf函数最终会通过调用fputc输出字符串到串口 */
// int fputc(int ch, FILE *f)
//{
//     while ((USART1->ISR & 0X40) == 0);    /* 等待上一个字符发送完成 */
//
//     USART1->TDR = (uint8_t)ch;            /* 将要发送的字符 ch 写入到DR寄存器 */
//     return ch;
// }
//
// #endif

/********************************禁止半主机结束********************************/

////方案三：选择use MicroLIB，重定向，直接使用printf

// 重定向 fputc 函数，目的是使用 printf 函数
// 入口参数：ch - 要输出的字符，f - 文件指针（这里用不到）
// 返 回 值：正常时返回字符，出错时返回 EOF（-1）
// int fputc(int ch, FILE *f)
//{
//	HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);	// 发送单字节数据
//	return (ch);
// }

/**
 * @brief  将多个int16_t数据转换为int8_t数组
 * @param  输入的int16_t数组
 * @param	int16_t数组的长度
 * @param  输出的int8_t数组
 * @retval 输出数组长度为输入数组长度的两位，且低位在前高位在后
 */
void convert_to_int8_array(int16_t *input_array, size_t length, int8_t *output_array)
{
  // 遍历输入数组并进行转换
  for (size_t i = 0; i < length; i++)
  {
    output_array[2 * i] = (int8_t)(input_array[i] & 0x00FF);            // 低字节
    output_array[2 * i + 1] = (int8_t)((input_array[i] >> 8) & 0x00FF); // 高字节
  }
}
/**
 * @brief  从缓冲区提取浮点数
 * @param  int8_t的数组
 * @param 	数组起始下标
 * @retval float类型占4byte，4byte一读
 */
float extract_float_from_buffer(int8_t *buffer, uint16_t start_index)
{
  float value;
  memcpy(&value, &buffer[start_index], sizeof(float));
  return value;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  static uint8_t flag = 0;	//0：未找到头 1：找到头
  BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult;

  if (huart->Instance == USART1) // 检查是否为串口1
  {
    // 搜寻报文头，接收剩下数据
    if (flag == 0)
    {
      if ((uint8_t)rxBuffer[0] == 0xAB)
      {
        if ((uint8_t)rxBuffer[1] == 0x54)
        {
          flag = 1; // 回调内部指示找到报文头
//          HAL_UART_Receive_IT(&huart1, (uint8_t *)&rxBuffer[1], 39);
					
					xResult = xEventGroupSetBitsFromISR(
          UsartReceiveEventHandle, /* The event group being updated. */
          BIT_1,                   /* The bits being set. */
          &xHigherPriorityTaskWoken);
        }
				else
				{
					xResult = xEventGroupSetBitsFromISR(
          UsartReceiveEventHandle, /* The event group being updated. */
          BIT_0,                   /* The bits being set. */
          &xHigherPriorityTaskWoken);
				}
      }
			else
			{
					xResult = xEventGroupSetBitsFromISR(
          UsartReceiveEventHandle, /* The event group being updated. */
          BIT_0,                   /* The bits being set. */
          &xHigherPriorityTaskWoken);
			}
    }
    // 接收完一组数据，关闭惯导报文接收，等待数据处理完成
    else if (flag == 1)
    {
//			__HAL_UART_DISABLE_IT(&huart1,UART_IT_RXNE);
			
      xResult = xEventGroupSetBitsFromISR(
          UsartReceiveEventHandle, /* The event group being updated. */
          BIT_2,                   /* The bits being set. */
          &xHigherPriorityTaskWoken);

      flag = 0; // 复位
    }
  }

  //	   if (huart->Instance == USART2)
  //		{
  //        ec_rxBuffer[ec_rxIndex++] = huart->Instance->DR;  // 先将接收的数据存入缓冲区
  //
  //        if(ec_rxBuffer[ec_rxIndex] == '\n') // 检查最近接收的字节是否是换行符
  //				{
  ////            ec_rxBuffer[ec_rxIndex + 1] = '\0';  // 添加字符串结束符
  //            if (strstr((char *)ec_rxBuffer, "OK") != NULL)
  //						{
  //                commandSuccess = 1;  // 收到OK
  ////								printf("set success");
  //            }
  //						else
  //						{
  //                commandSuccess = 0;  // 没有收到OK
  ////								printf("set fail");
  //            }
  //            ec_rxIndex = 0;  // 重置缓冲区索引
  //        }
  //        // 重新启用接收中断，接收下一个字节
  //         HAL_UART_Receive_IT(huart, (uint8_t *)&ec_rxBuffer[ec_rxIndex], 1);
  //    }
}

#endif
