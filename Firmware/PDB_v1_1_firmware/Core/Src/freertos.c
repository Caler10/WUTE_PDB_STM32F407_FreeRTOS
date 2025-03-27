/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//mx
#include "tim.h"
#include "usart.h"
//bsp
#include "hsd_drive.h"
#include "mycan.h"
#include "oled.h"
#include "oledfont.h"	
#include "myusart.h"
#include "voltage_monitor.h"
#include "standby.h"
//app
#include "monitor_app.h"
#include "can_app.h"
#include "oled_app.h"
//freertos
#include "queue.h"
#include "stream_buffer.h"
#include "event_groups.h"
#include "semphr.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticEventGroup_t osStaticEventGroupDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for CanErrorSendTas */
osThreadId_t CanErrorSendTasHandle;
const osThreadAttr_t CanErrorSendTas_attributes = {
  .name = "CanErrorSendTas",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for BATdetectTask */
osThreadId_t BATdetectTaskHandle;
const osThreadAttr_t BATdetectTask_attributes = {
  .name = "BATdetectTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for OLEDmonitorTask */
osThreadId_t OLEDmonitorTaskHandle;
const osThreadAttr_t OLEDmonitorTask_attributes = {
  .name = "OLEDmonitorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for CanDataSendTask */
osThreadId_t CanDataSendTaskHandle;
const osThreadAttr_t CanDataSendTask_attributes = {
  .name = "CanDataSendTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for CANreceiveTask */
osThreadId_t CANreceiveTaskHandle;
const osThreadAttr_t CANreceiveTask_attributes = {
  .name = "CANreceiveTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for HSDctrlTask */
osThreadId_t HSDctrlTaskHandle;
const osThreadAttr_t HSDctrlTask_attributes = {
  .name = "HSDctrlTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for DRSctrlTask */
osThreadId_t DRSctrlTaskHandle;
const osThreadAttr_t DRSctrlTask_attributes = {
  .name = "DRSctrlTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for WakeupTask */
osThreadId_t WakeupTaskHandle;
const osThreadAttr_t WakeupTask_attributes = {
  .name = "WakeupTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for StandbyTask */
osThreadId_t StandbyTaskHandle;
const osThreadAttr_t StandbyTask_attributes = {
  .name = "StandbyTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime7,
};
/* Definitions for HSD1diagnosTask */
osThreadId_t HSD1diagnosTaskHandle;
const osThreadAttr_t HSD1diagnosTask_attributes = {
  .name = "HSD1diagnosTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for HSD2diagnosTask */
osThreadId_t HSD2diagnosTaskHandle;
const osThreadAttr_t HSD2diagnosTask_attributes = {
  .name = "HSD2diagnosTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for CPUTask */
osThreadId_t CPUTaskHandle;
const osThreadAttr_t CPUTask_attributes = {
  .name = "CPUTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LED_Toggle */
osThreadId_t LED_ToggleHandle;
const osThreadAttr_t LED_Toggle_attributes = {
  .name = "LED_Toggle",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for IMU2CAN */
osThreadId_t IMU2CANHandle;
const osThreadAttr_t IMU2CAN_attributes = {
  .name = "IMU2CAN",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime7,
};
/* Definitions for AVDataQueue */
osMessageQueueId_t AVDataQueueHandle;
const osMessageQueueAttr_t AVDataQueue_attributes = {
  .name = "AVDataQueue"
};
/* Definitions for AVErrorQueue */
osMessageQueueId_t AVErrorQueueHandle;
const osMessageQueueAttr_t AVErrorQueue_attributes = {
  .name = "AVErrorQueue"
};
/* Definitions for CanReceiveQueue */
osMessageQueueId_t CanReceiveQueueHandle;
const osMessageQueueAttr_t CanReceiveQueue_attributes = {
  .name = "CanReceiveQueue"
};
/* Definitions for OLEDtoCANerrorSend */
osMessageQueueId_t OLEDtoCANerrorSendHandle;
const osMessageQueueAttr_t OLEDtoCANerrorSend_attributes = {
  .name = "OLEDtoCANerrorSend"
};
/* Definitions for OLEDoccupiedSem */
osSemaphoreId_t OLEDoccupiedSemHandle;
const osSemaphoreAttr_t OLEDoccupiedSem_attributes = {
  .name = "OLEDoccupiedSem"
};
/* Definitions for MonitorAnalyzeEvent */
osEventFlagsId_t MonitorAnalyzeEventHandle;
const osEventFlagsAttr_t MonitorAnalyzeEvent_attributes = {
  .name = "MonitorAnalyzeEvent"
};
/* Definitions for CanReceiveEvent */
osEventFlagsId_t CanReceiveEventHandle;
const osEventFlagsAttr_t CanReceiveEvent_attributes = {
  .name = "CanReceiveEvent"
};
/* Definitions for UsartReceiveEvent */
osEventFlagsId_t UsartReceiveEventHandle;
osStaticEventGroupDef_t UsartReceiveEventControlBlock;
const osEventFlagsAttr_t UsartReceiveEvent_attributes = {
  .name = "UsartReceiveEvent",
  .cb_mem = &UsartReceiveEventControlBlock,
  .cb_size = sizeof(UsartReceiveEventControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Start_CAN_Error_Send_Task(void *argument);
void Start_BAT_Detect_Task(void *argument);
void Start_OLED_Monitor_Task(void *argument);
void Start_CAN_Data_Send_Task(void *argument);
void Start_CAN_Receive_Task(void *argument);
void Start_HSD_Ctrl_Task(void *argument);
void Start_DRS_Ctrl_Task(void *argument);
void Start_Wakeup_Task(void *argument);
//void Start_Standby_Task(void *argument);
void Start_HSD1_Diagnose_Task(void *argument);
void Start_HSD2_Diagnose_Task(void *argument);
void Start_CPU_Task(void *argument);
void Start_LEDToggle_Task(void *argument);
void Start_IMU2CAN_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	
	uint8_t i=0;	//起始0个错误
//	unsigned char Oled_First_Text[8]="Error  0";	
	extern uint8_t trigger;
	
	myprintf("into freertos init\r\n");
	
  //OLED init
//	OLED_Init();
//	OLED_DrawBMP (0, 0,128, 8,(unsigned char*)WUTE);	
	

//	HAL_UART_Receive_IT(&huart1, (uint8_t *)rxBuffer, 1);//恢复接收中断
	
//	HAL_Delay(100);
//	osDelay(1000);		//用这个就跳到任务去了
	
	
  //HSD open
	HSD_Output_Enable_HSDs(1);
	__HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, 199); //配置比较寄存器(相对于200的自动重装载值)
	HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);		 //开启PWM输出（FAN1）
	__HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, 199); //配置比较寄存器(相对于200的自动重装载值)
	HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);		 //开启PWM输出（FAN2）
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 99); //配置比较寄存器(相对于100的自动重装载值)
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);		 //开启PWM输出（尾灯）
	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 100); //配置比较寄存器(相对于200的自动重装载值)
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);		 //开启PWM输出（水泵）
	
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, 0);	//BUZZER_PWR_EN不响
//	HAL_Delay(10);

	//不要用这个函数单个通道控制，这个驱动有问题（建议直接GPIO写PIN，最高效）
//	HSD_Output_Enable_HSDPinx(0xA1,0);	
	
	//oled into monitor state
//	OLED_FullyClear();
//	Oled_First_Text[7]=i+48;		//需要赋值字符‘'，选择ascii码操作
//	OLED_ShowStr(0, 0, (unsigned char*)Oled_First_Text ,1 );
	
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of OLEDoccupiedSem */
  OLEDoccupiedSemHandle = osSemaphoreNew(32, 32, &OLEDoccupiedSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of AVDataQueue */
  AVDataQueueHandle = osMessageQueueNew (32, sizeof(uint8_t), &AVDataQueue_attributes);

  /* creation of AVErrorQueue */
  AVErrorQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &AVErrorQueue_attributes);

  /* creation of CanReceiveQueue */
  CanReceiveQueueHandle = osMessageQueueNew (16, sizeof(uint8_t), &CanReceiveQueue_attributes);

  /* creation of OLEDtoCANerrorSend */
  OLEDtoCANerrorSendHandle = osMessageQueueNew (8, sizeof(uint16_t), &OLEDtoCANerrorSend_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of CanErrorSendTas */
  CanErrorSendTasHandle = osThreadNew(Start_CAN_Error_Send_Task, NULL, &CanErrorSendTas_attributes);

  /* creation of BATdetectTask */
  BATdetectTaskHandle = osThreadNew(Start_BAT_Detect_Task, NULL, &BATdetectTask_attributes);

  /* creation of OLEDmonitorTask */
  OLEDmonitorTaskHandle = osThreadNew(Start_OLED_Monitor_Task, NULL, &OLEDmonitorTask_attributes);

  /* creation of CanDataSendTask */
  CanDataSendTaskHandle = osThreadNew(Start_CAN_Data_Send_Task, NULL, &CanDataSendTask_attributes);

  /* creation of CANreceiveTask */
  CANreceiveTaskHandle = osThreadNew(Start_CAN_Receive_Task, NULL, &CANreceiveTask_attributes);

  /* creation of HSDctrlTask */
  HSDctrlTaskHandle = osThreadNew(Start_HSD_Ctrl_Task, NULL, &HSDctrlTask_attributes);

  /* creation of DRSctrlTask */
  DRSctrlTaskHandle = osThreadNew(Start_DRS_Ctrl_Task, NULL, &DRSctrlTask_attributes);

  /* creation of WakeupTask */
  WakeupTaskHandle = osThreadNew(Start_Wakeup_Task, NULL, &WakeupTask_attributes);

  /* creation of StandbyTask */
//  StandbyTaskHandle = osThreadNew(Start_Standby_Task, NULL, &StandbyTask_attributes);

  /* creation of HSD1diagnosTask */
  HSD1diagnosTaskHandle = osThreadNew(Start_HSD1_Diagnose_Task, NULL, &HSD1diagnosTask_attributes);

  /* creation of HSD2diagnosTask */
  HSD2diagnosTaskHandle = osThreadNew(Start_HSD2_Diagnose_Task, NULL, &HSD2diagnosTask_attributes);

  /* creation of CPUTask */
  CPUTaskHandle = osThreadNew(Start_CPU_Task, NULL, &CPUTask_attributes);

  /* creation of LED_Toggle */
  LED_ToggleHandle = osThreadNew(Start_LEDToggle_Task, NULL, &LED_Toggle_attributes);

  /* creation of IMU2CAN */
  IMU2CANHandle = osThreadNew(Start_IMU2CAN_Task, NULL, &IMU2CAN_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of MonitorAnalyzeEvent */
  MonitorAnalyzeEventHandle = osEventFlagsNew(&MonitorAnalyzeEvent_attributes);

  /* creation of CanReceiveEvent */
  CanReceiveEventHandle = osEventFlagsNew(&CanReceiveEvent_attributes);

  /* creation of UsartReceiveEvent */
  UsartReceiveEventHandle = osEventFlagsNew(&UsartReceiveEvent_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_Start_CAN_Error_Send_Task */
/**
  * @brief  Function implementing the CanErrorSendTas thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Start_CAN_Error_Send_Task */
void Start_CAN_Error_Send_Task(void *argument)
{
  /* USER CODE BEGIN Start_CAN_Error_Send_Task */
  /* Infinite loop */
  CAN_Error_Send_App();
  /* USER CODE END Start_CAN_Error_Send_Task */
}

/* USER CODE BEGIN Header_Start_BAT_Detect_Task */
/**
* @brief Function implementing the BATdetectTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_BAT_Detect_Task */
void Start_BAT_Detect_Task(void *argument)
{
  /* USER CODE BEGIN Start_BAT_Detect_Task */
	BAT_Monitor_App();		//对电压数据监测与处理，唤起其他操作
  /* USER CODE END Start_BAT_Detect_Task */
}

/* USER CODE BEGIN Header_Start_OLED_Monitor_Task */
/**
* @brief Function implementing the OLEDmonitorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_OLED_Monitor_Task */
void Start_OLED_Monitor_Task(void *argument)
{
  /* USER CODE BEGIN Start_OLED_Monitor_Task */
	Oled_App();
  /* USER CODE END Start_OLED_Monitor_Task */
}

/* USER CODE BEGIN Header_Start_CAN_Data_Send_Task */
/**
* @brief Function implementing the CanDataSendTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_CAN_Data_Send_Task */
void Start_CAN_Data_Send_Task(void *argument)
{
  /* USER CODE BEGIN Start_CAN_Data_Send_Task */
  CAN_Data_Send_App();
  /* USER CODE END Start_CAN_Data_Send_Task */
}

/* USER CODE BEGIN Header_Start_CAN_Receive_Task */
/**
* @brief Function implementing the CANreceiveTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_CAN_Receive_Task */
void Start_CAN_Receive_Task(void *argument)
{
  /* USER CODE BEGIN Start_CAN_Receive_Task */
	CAN_Recv_App();
  /* USER CODE END Start_CAN_Receive_Task */
}

/* USER CODE BEGIN Header_Start_HSD_Ctrl_Task */
/**
* @brief Function implementing the HSDctrlTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_HSD_Ctrl_Task */
void Start_HSD_Ctrl_Task(void *argument)
{
  /* USER CODE BEGIN Start_HSD_Ctrl_Task */
	uint8_t command;
	EventBits_t uxBits;			 // 读取事件组的返回值（已配置事件组宽度为24bit）
	
	//延时3秒后关闭
	osDelay(3000);
	//下面几个通道默认关闭
//	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, 0);		//TAILLIGHT_PWR_EN
//	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);	//BUZZER_PWR_EN
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 0);		//PUMP_PWR_EN
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);		//FAN1_PWR_EN(控制FAN3)
//	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, 0);		//FAN3_PWR_EN（现FAN1）
//	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, 0);		//FAN2_PWR_EN
	
	__HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, 0); //配置比较寄存器(相对于200的自动重装载值)
	HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);		 //开启PWM输出（FAN1）
	__HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, 0); //配置比较寄存器(相对于200的自动重装载值)
	HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);		 //开启PWM输出（FAN2）
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0); //配置比较寄存器(相对于100的自动重装载值)
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);		 //开启PWM输出（尾灯）
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 150); //配置比较寄存器(相对于200的自动重装载值)
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);		 //开启PWM输出（水泵）
	
	//完成上电初始化后才开启CAN收发
	CAN_Init();
	
  /* Infinite loop */
  for(;;)
  {
		uxBits = xEventGroupWaitBits(
			CanReceiveEventHandle,		   /* 大电流HSD相关事件（ADC相对通道转换完成（4bit）、发生错误（1bit）、一组处理完成已发出数据（1bit））句柄 */
			BIT_10|BIT_11 , /* 指代VCU、仪表发来can控制指令的事件(设置了4位) */
			pdTRUE,						   /* 事件标志位复位后退出 */
			pdFALSE,					   /* 或关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);				   /* 等无限久 */

	//先不要用这个函数单个通道控制，这个驱动有问题（建议直接GPIO写PIN，最高效）
//	HSD_Output_Enable_HSDPinx(0x91, 0);
		
//	if( ( uxBits & BIT_8 ) != 0 )	//水泵开
//  {
//      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
//		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 1);		//PUMP_PWR_EN
//  }
//	if( ( uxBits & BIT_9 ) != 0 )	//水泵关
//  {
//      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
//		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 0);		//PUMP_PWR_EN
//  }
	if( ( uxBits & BIT_10 ) != 0 )	//蜂鸣器开
  {
      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, 1);	//BUZZER_PWR_EN(改为L1_5V_Backup)
  }
	if( ( uxBits & BIT_11 ) != 0 )	//蜂鸣器关
  {
      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, 0);	//BUZZER_PWR_EN(改为L1_5V_Backup)
  }
//	if( ( uxBits & BIT_12 ) != 0 )	//水泵开
//  {
//      /* xEventGroupWaitBits() returned because just BIT_0 was set. */
//		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, 1);		//TAILLIGHT_PWR_EN  
//  }
//  if( ( uxBits & BIT_13 ) != 0 )	//水泵关
//  {
//      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
//		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, 0);		//TAILLIGHT_PWR_EN
//  }

	//回调执行任务，不延时
//    osDelay(1000);
  }
  /* USER CODE END Start_HSD_Ctrl_Task */
}

/* USER CODE BEGIN Header_Start_DRS_Ctrl_Task */
/**
* @brief Function implementing the DRSctrlTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_DRS_Ctrl_Task */
void Start_DRS_Ctrl_Task(void *argument)
{
  /* USER CODE BEGIN Start_DRS_Ctrl_Task */
	//
	uint16_t compareValue=0 ; 
	
	BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult;
	BaseType_t xReturn = pdPASS; /* 定义一个创建信息返回值，默认为pdPASS */
	EventBits_t uxBits;			 // 读取事件组的返回值（已配置事件组宽度为24bit）
	
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, 1);	//电平转换芯片OE脚，置高开启
  /* Infinite loop */
  for(;;)
  {
		//VCU、仪表发来控制信号（现在还只是设置DRS开关）
		uxBits = xEventGroupWaitBits(
			CanReceiveEventHandle, /* 大电流HSD相关事件（ADC相对通道转换完成（4bit）、发生错误（1bit）、一组处理完成已发出数据（1bit））句柄 */
			BIT_16|BIT_17,	   //VCU控制DRS事件位为BIT_12|BIT_13
			pdTRUE,				   /* 事件标志位复位后退出 */
			pdFALSE,			   /* 或关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);		   /* 等无限久（既然能过，必然是收到了事件，所以下面不再if判断是否事件真置位） */
		
		if (( uxBits & BIT_16 ) != 0) // DRS开启(初始化时预分频后频率为100KMz 100计数后频率999HZ，默认关闭
				{
					//设置频率为999Hz 占空比为50%
					__HAL_TIM_SetAutoreload(&htim4 , 99);
					__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, 50); // 配置比较寄存器(相对于100的自动重装载值)
					HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);		   //开启PWM输出
					myprintf("DRS start\r\n");
				}
		else if (( uxBits & BIT_17 ) != 0) // DRS关闭 对应999Hz
				{
					//设置频率为999Hz 占空比为50%
					__HAL_TIM_SetAutoreload(&htim4 , 299);
					__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, 150); // 配置比较寄存器(相对于300的自动重装载值)
					HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);		 //开启PWM输出
					myprintf("DRS close\r\n");
				}
		
		//回调任务，不延时
//    osDelay(1000);
  }
  /* USER CODE END Start_DRS_Ctrl_Task */
}

/* USER CODE BEGIN Header_Start_Wakeup_Task */
/**
* @brief Function implementing the WakeupTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_Wakeup_Task */
void Start_Wakeup_Task(void *argument)
{
  /* USER CODE BEGIN Start_Wakeup_Task */
	Wakeup_App();
  /* USER CODE END Start_Wakeup_Task */
}

/* USER CODE BEGIN Header_Start_Standby_Task */
/**
* @brief Function implementing the StandbyTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_Standby_Task */
void Start_Standby_Task(void *argument)
{
  /* USER CODE BEGIN Start_Standby_Task */
//  Standby_App();
	osDelay(1000);
  /* USER CODE END Start_Standby_Task */
}

/* USER CODE BEGIN Header_Start_HSD1_Diagnose_Task */
/**
* @brief Function implementing the HSD1diagnosTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_HSD1_Diagnose_Task */
void Start_HSD1_Diagnose_Task(void *argument)
{
  /* USER CODE BEGIN Start_HSD1_Diagnose_Task */
	//其优先级必须最低，等都处理完再执行，否则错误码未发出就覆盖了（HSD1\HSD2\HSD3共用同一个错误码发送队列，而CAN发送任务仅仅用一个uint16_t 变量接收）
	HSD1_Monitor_App();		//对HSD1采集的电流数据监测与处理，唤起其他操作
  /* USER CODE END Start_HSD1_Diagnose_Task */
}

/* USER CODE BEGIN Header_Start_HSD2_Diagnose_Task */
/**
* @brief Function implementing the HSD2diagnosTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_HSD2_Diagnose_Task */
void Start_HSD2_Diagnose_Task(void *argument)
{
  /* USER CODE BEGIN Start_HSD2_Diagnose_Task */
	HSD2_Monitor_App();		//对HSD2采集的电流数据监测与处理，唤起其他操作
  /* USER CODE END Start_HSD2_Diagnose_Task */
}

/* USER CODE BEGIN Header_Start_CPU_Task */
/**
* @brief Function implementing the CPUTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_CPU_Task */
void Start_CPU_Task(void *argument)
{
  /* USER CODE BEGIN Start_CPU_Task */
	
//	uint8_t CPU_RunInfo[400];		//保存任务运行时间信息
	
  /* Infinite loop */
  for(;;)
  {
//		memset(CPU_RunInfo,0,400);				//信息缓冲区清零
//    
//    vTaskList((char *)&CPU_RunInfo);  //获取任务运行时间信息
//    
//    myprintf("---------------------------------------------\r\n");
//    myprintf("任务名      任务状态 优先级   剩余栈 任务序号\r\n");
//    myprintf("%s", CPU_RunInfo);
//    myprintf("---------------------------------------------\r\n");
//    
//    memset(CPU_RunInfo,0,400);				//信息缓冲区清零
//    
//    vTaskGetRunTimeStats((char *)&CPU_RunInfo);
//    
//    myprintf("任务名       运行计数         使用率\r\n");
//    myprintf("%s", CPU_RunInfo);
//    myprintf("---------------------------------------------\r\n\n");
//    vTaskDelay(1000);   //延时500个tick
    osDelay(10000);
  }
  /* USER CODE END Start_CPU_Task */
}

/* USER CODE BEGIN Header_Start_LEDToggle_Task */
/**
* @brief Function implementing the LED_Toggle thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_LEDToggle_Task */
void Start_LEDToggle_Task(void *argument)
{
  /* USER CODE BEGIN Start_LEDToggle_Task */
  /* Infinite loop */
  for(;;)
  {
		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_12);
    osDelay(1000);
  }
  /* USER CODE END Start_LEDToggle_Task */
}

/* USER CODE BEGIN Header_Start_IMU2CAN_Task */
/**
* @brief Function implementing the IMU2CAN thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_IMU2CAN_Task */
void Start_IMU2CAN_Task(void *argument)
{
  /* USER CODE BEGIN Start_IMU2CAN_Task */
	//该任务周期执行，高优先级
	int8_t rxBuffer1[16];
	int16_t txcanBuffer[TX_CAN_BUFFER_SIZE]; // 待发送数据区
	int8_t txcanBuffer1[8];
	uint8_t txcanIndex = 0;

	EventBits_t uxBits;
	
//	HAL_UART_Receive_IT(&huart1, (uint8_t *)rxBuffer, 1);	
	HAL_UART_Receive_DMA(&huart1, (uint8_t *)rxBuffer, 2);
	
  /* Infinite loop */
  for(;;)
  {
		//等待
		uxBits = xEventGroupWaitBits(
			UsartReceiveEventHandle, /* 大电流HSD相关事件（ADC相对通道转换完成（4bit）、发生错误（1bit）、一组处理完成已发出数据（1bit））句柄 */
			BIT_0|BIT_1|BIT_2 ,   /* HSD1\HSD2\BAT完成一包数据处理,对应事件位BIT_20\BIT_21\BIT22(BIT从BIT_0开始);和故障出现事件 */
			pdTRUE,					   /* 事件标志位复位后退出 */
			pdFALSE,				   /* 或关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);			   /* 无限等，否则直接读事件组返回（改为等无限久） */
		
		//未找到头
		if((uxBits & BIT_0) != 0)
		{
			HAL_UART_Receive_DMA(&huart1, (uint8_t *)&rxBuffer[0], 2);
		}
		//找到头
		else if((uxBits & BIT_1) != 0)
		{
			HAL_UART_Receive_DMA(&huart1, (uint8_t *)&rxBuffer[2], 38);
		}
		//一组数据接收完毕，数据处理
		else if((uxBits & BIT_2) != 0)
		{
//			uint8_t i =  10;
				uint8_t j = 0;

				memcpy(&rxBuffer1[0], &rxBuffer[11], 4);
				memcpy(&rxBuffer1[4], &rxBuffer[19], 12);
					
				// 反转每个4个元素的块
//				for (i = 3; i >= 0; i--) {
//						rxBuffer2[k++] = rxBuffer1[i];
//				}
//				for (i = 7; i >= 4; i--) {
//								rxBuffer2[k++] = rxBuffer1[i];
//				}
//				for (i = 11; i >= 8; i--) {
//								rxBuffer2[k++] = rxBuffer1[i];
//				}
//				for (i = 15;i >= 12; i--){
//								rxBuffer2[k++] = rxBuffer1[i];
//				}
//					
				//压缩数据
				for(j=0;j<16;j+=4)
					{
							txcanBuffer[txcanIndex] = (uint16_t) (extract_float_from_buffer(rxBuffer1,j) * 100/1);
							txcanIndex++;
					}
				convert_to_int8_array(txcanBuffer,4,txcanBuffer1);//16bit->低八位在前，高八位在后
						
				CAN_Transmit((const void *)&txcanBuffer1,8,0x0F146D72);//发送CAN报文

				rxIndex = 0;
				txcanIndex = 0;
						
				//一组数据处理完成，再次接收
//				__HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE);//打开中断
//				HAL_UART_Receive_IT(&huart1, (uint8_t *)rxBuffer, 1);//恢复接收中断
					
				HAL_UART_Receive_DMA(&huart1, (uint8_t *)&rxBuffer[0], 2);
		}
//    osDelay(50);	//配置转发速率
  }
  /* USER CODE END Start_IMU2CAN_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

