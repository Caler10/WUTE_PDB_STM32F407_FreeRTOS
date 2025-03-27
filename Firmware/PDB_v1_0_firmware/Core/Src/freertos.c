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
void Start_Standby_Task(void *argument);
void Start_HSD1_Diagnose_Task(void *argument);
void Start_HSD2_Diagnose_Task(void *argument);
void Start_CPU_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	
	uint8_t i=0;	//��ʼ0������
	unsigned char Oled_First_Text[8]="Error  0";	
	extern uint8_t trigger;
	
	myprintf("into freertos init\r\n");
	
  //OLED init
	OLED_Init();
//	OLED_DrawBMP (0, 0,128, 8,(unsigned char*)WUTE);	
	
	//include can init��filter_config��start can_tx and en can_rx fifo
	CAN_Init();
	
//	HAL_Delay(100);
//	osDelay(1000);		//���������������ȥ��

  //HSD open
	HSD_Output_Enable_HSDs(1);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 0);	//BUZZER_PWR_EN����
	
	//��Ҫ�������������ͨ�����ƣ�������������⣨����ֱ��GPIOдPIN�����Ч��
//	HSD_Output_Enable_HSDPinx(0xA1,0);	
	
	//oled into monitor state
	OLED_FullyClear();
	Oled_First_Text[7]=i+48;		//��Ҫ��ֵ�ַ���'��ѡ��ascii�����
	OLED_ShowStr(0, 0, (unsigned char*)Oled_First_Text ,1 );
	
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
  StandbyTaskHandle = osThreadNew(Start_Standby_Task, NULL, &StandbyTask_attributes);

  /* creation of HSD1diagnosTask */
  HSD1diagnosTaskHandle = osThreadNew(Start_HSD1_Diagnose_Task, NULL, &HSD1diagnosTask_attributes);

  /* creation of HSD2diagnosTask */
  HSD2diagnosTaskHandle = osThreadNew(Start_HSD2_Diagnose_Task, NULL, &HSD2diagnosTask_attributes);

  /* creation of CPUTask */
  CPUTaskHandle = osThreadNew(Start_CPU_Task, NULL, &CPUTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of MonitorAnalyzeEvent */
  MonitorAnalyzeEventHandle = osEventFlagsNew(&MonitorAnalyzeEvent_attributes);

  /* creation of CanReceiveEvent */
  CanReceiveEventHandle = osEventFlagsNew(&CanReceiveEvent_attributes);

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
	BAT_Monitor_App();		//�Ե�ѹ���ݼ���봦��������������
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
	EventBits_t uxBits;			 // ��ȡ�¼���ķ���ֵ���������¼�����Ϊ24bit��
	
	//��ʱ3���ر�
	osDelay(3000);
	//���漸��ͨ��Ĭ�Ϲر�
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, 0);		//TAILLIGHT_PWR_EN ��ΪL2_24V_Backup
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 0);	//BUZZER_PWR_EN
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, 0);		//PUMP_PWR_EN
//	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, 0);		//FAN1_PWR_EN
//	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, 0);		//FAN3_PWR_EN
//	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, 0);		//FAN2_PWR_EN
	
  /* Infinite loop */
  for(;;)
  {
		uxBits = xEventGroupWaitBits(
			CanReceiveEventHandle,		   /* �����HSD����¼���ADC���ͨ��ת����ɣ�4bit������������1bit����һ�鴦������ѷ������ݣ�1bit������� */
			BIT_8|BIT_9|BIT_10|BIT_11|BIT_12|BIT_13 , /* ָ��VCU���Ǳ���can����ָ����¼�(������4λ) */
			pdTRUE,						   /* �¼���־λ��λ���˳� */
			pdFALSE,					   /* ���ϵ�����ȶ���λ�˲Ŵ����¼������� */
			portMAX_DELAY);				   /* �����޾� */

	//�Ȳ�Ҫ�������������ͨ�����ƣ�������������⣨���⽨��ֱ��GPIOдPIN�����Ч��
//	HSD_Output_Enable_HSDPinx(0x91, 0);
  if( ( uxBits & BIT_8 ) != 0 )	//β�ƿ�
  {
      /* xEventGroupWaitBits() returned because just BIT_0 was set. */
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, 1);		//TAILLIGHT_PWR_EN  
  }
  if( ( uxBits & BIT_9 ) != 0 )	//β�ƹ�
  {
      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, 0);		//TAILLIGHT_PWR_EN
  }
	if( ( uxBits & BIT_10 ) != 0 )	//ˮ�ÿ�
  {
      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, 1);		//PUMP_PWR_EN
  }
	if( ( uxBits & BIT_11 ) != 0 )	//ˮ�ù�
  {
      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, 0);		//PUMP_PWR_EN
  }
	if( ( uxBits & BIT_12 ) != 0 )	//��������
  {
      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 1);	//BUZZER_PWR_EN
  }
	if( ( uxBits & BIT_13 ) != 0 )	//��������
  {
      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 0);	//BUZZER_PWR_EN
  }
//	else if( ( uxBits & BIT_14 ) != 0 )	//DRS��
//  {
//      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
//		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, 1);		//DRS_PWR_8V_EN
//  }
//	else if( ( uxBits & BIT_15 ) != 0 )	//DRS��
//  {
//      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
//		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, 0);		//DRS_PWR_8V_EN
//  }
//	else if( ( uxBits & BIT_16 ) != 0 )	//FAN2��
//  {
//      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
//		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 1);	//BUZZER_PWR_EN
//  }
//	else if( ( uxBits & BIT_17 ) != 0 )	//FAN2��
//  {
//      /* xEventGroupWaitBits() returned because just BIT_4 was set. */
//		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 0);	//BUZZER_PWR_EN
//  }
	//�ص����񣬲���ʱ
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
	BaseType_t xReturn = pdPASS; /* ����һ��������Ϣ����ֵ��Ĭ��ΪpdPASS */
	EventBits_t uxBits;			 // ��ȡ�¼���ķ���ֵ���������¼�����Ϊ24bit��
	
  /* Infinite loop */
  for(;;)
  {
		//VCU���Ǳ��������źţ����ڻ�ֻ������DRS���أ�
		uxBits = xEventGroupWaitBits(
			CanReceiveEventHandle, /* �����HSD����¼���ADC���ͨ��ת����ɣ�4bit������������1bit����һ�鴦������ѷ������ݣ�1bit������� */
			BIT_14|BIT_15,	   /* ���ӣ�δ�� */
			pdTRUE,				   /* �¼���־λ��λ���˳� */
			pdFALSE,			   /* ���ϵ�����ȶ���λ�˲Ŵ����¼������� */
			portMAX_DELAY);		   /* �����޾ã���Ȼ�ܹ�����Ȼ���յ����¼����������治��if�ж��Ƿ��¼�����λ�� */
		
		if (( uxBits & BIT_14 ) != 0) // DRS����
				{
					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 100); // ���ñȽϼĴ���(�����1000���Զ���װ��ֵ)
					HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);		   // ���⿪��������
				}
		if (( uxBits & BIT_15 ) != 0) // DRS�ر�
				{
					// �������õ�PWM����Ϊ0.02s��50Hz
					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0); // ���ñȽϼĴ���(�����1000���Զ���װ��ֵ)
					HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);		 // ���⿪��������
				}
		
		myprintf("DRS ctrl finish");
		
		//�ص����񣬲���ʱ
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
  Standby_App();
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
	//�����ȼ�������ͣ��ȶ���������ִ�У����������δ�����͸����ˣ�HSD1\HSD2\HSD3����ͬһ�������뷢�Ͷ��У���CAN�������������һ��uint16_t �������գ�
	HSD1_Monitor_App();		//��HSD1�ɼ��ĵ������ݼ���봦��������������
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
	HSD2_Monitor_App();		//��HSD2�ɼ��ĵ������ݼ���봦��������������
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
	
//	uint8_t CPU_RunInfo[400];		//������������ʱ����Ϣ
//	
  /* Infinite loop */
  for(;;)
  {
//		memset(CPU_RunInfo,0,400);				//��Ϣ����������
//    
//    vTaskList((char *)&CPU_RunInfo);  //��ȡ��������ʱ����Ϣ
//    
//    myprintf("---------------------------------------------\r\n");
//    myprintf("������      ����״̬ ���ȼ�   ʣ��ջ �������\r\n");
//    myprintf("%s", CPU_RunInfo);
//    myprintf("---------------------------------------------\r\n");
//    
//    memset(CPU_RunInfo,0,400);				//��Ϣ����������
//    
//    vTaskGetRunTimeStats((char *)&CPU_RunInfo);
//    
//    myprintf("������       ���м���         ʹ����\r\n");
//    myprintf("%s", CPU_RunInfo);
//    myprintf("---------------------------------------------\r\n\n");
//    vTaskDelay(1000);   //��ʱ500��tick
    osDelay(1000);
  }
  /* USER CODE END Start_CPU_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

