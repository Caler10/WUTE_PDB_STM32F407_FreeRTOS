/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

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
int count_one_bits(unsigned int value);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

                    //��MonitorAnalyzeEvent��	//CanReceiveEventHandle
#define BIT_0 1<<0		//HSD1ͨ��1 ADC DMA�������	//CAN����VCU�ص����յ��¼�
#define BIT_1 1<<1		//HSD2ͨ��2 ADC DMA�������	//CAN�����Ǳ�ص����յ��¼�
#define BIT_2 1<<2		//HSD2ͨ��3 ADC DMA�������  //
#define BIT_3 1<<3		//HSD2ͨ��4 ADC DMA�������	//
#define BIT_4 1<<4		//HSD1 ���ִ���		//
#define BIT_5 1<<5		//HSD1 ��������ɷ���,����		//
#define BIT_6 1<<6		//HSD1 һ�����ݷ������	//
#define BIT_7 1<<7		//HSD1 һ�������ϰ����,����	//

#define BIT_8 1<<8		//HSD2ͨ��1 ADC DMA�������	//
#define BIT_9 1<<9		//HSD2ͨ��2 ADC DMA�������	//
#define BIT_10 1<<10	//HSD2 ���ִ���	//
#define BIT_11 1<<11	//HSD2 ��������ɷ���,����	//
#define BIT_12 1<<12	//HSD2 һ�����ݷ������	//
#define BIT_13 1<<13	//HSD2 һ�������ϰ����,����	//

#define BIT_14 1<<14	//BAT ADC DMA�������	//
#define BIT_15 1<<15	//BAT ���ִ���	//
#define BIT_16 1<<16	//BAT ��������ɷ���,����		//
#define BIT_17 1<<17	//BAT һ�����ݷ������		//
#define BIT_18 1<<18	//BAT һ�������ϰ����,����		//
#define BIT_19 1<<19  //	//
#define BIT_20 1<<20  //	//
#define BIT_21 1<<21	//		//
#define BIT_22 1<<22	//		//
#define BIT_23 1<<23	//OLED��CAN���������		//����Ϊ��������־λ

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
