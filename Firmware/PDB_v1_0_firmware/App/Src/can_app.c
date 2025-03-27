// CAN收发的上层应用
// 问题：清理CAN接收缓冲区、CAN频繁收取中断打断

#if 1

// mx
#include "can.h"
#include "tim.h" //为控制DRS
// bsp
#include "mycan.h"
#include "myusart.h"
#include "hsd_drive.h"
// app
#include "can_app.h"
// freertos
#include "cmsis_os.h" //已引用FreeRTOS.h
#include "queue.h"
#include "stream_buffer.h"
#include "event_groups.h"
#include "semphr.h"

extern CAN_RxHeaderTypeDef RxHeader;
extern CAN_TxHeaderTypeDef TxHeader;

extern EventGroupHandle_t MonitorAnalyzeEventHandle;
extern EventGroupHandle_t CanReceiveEventHandle;
extern QueueHandle_t CanReceiveQueueHandle;
extern QueueHandle_t AVErrorQueueHandle;
extern QueueHandle_t AVDataQueueHandle;
extern QueueHandle_t OLEDtoCANerrorSendHandle;

// CAN发送采用同步截取打包发送，就算各采集频次不一样，也同步地众一包发送
// 发送归发送，各诊断检测频率归各，据事件三者齐了就发
void CAN_Data_Send_App(void)
{
	uint8_t i, flag1 = 0, flag2 = 0, flag3 = 0; // flag1:HSD1处理完成至少一包 flag2：HSD2处理好至少一包 flag3：BAT处理好至少一包
	uint8_t AV_Data[32];						// 电流电压缓存数组，前30个单元存储ADC采集的电流数据*10（保留小数点后一位）；后2单元存储ADC采集的电压数据-20（保留一位小数）
	EventBits_t uxBits, uxBits1;				// 读取事件组的返回值（已配置事件组宽度为24bit）/uxBits1做另一事件处理函数的返回值接收

	/* Infinite loop */
	for (;;) // 出问题原因：多个事件set\clear复用同一个uxBits
	{
		// 各数据包处理完成、出现错误（最后不用再清除故障事件了，这里收到后已经清理了一次；另外，uxBits是返回事件组里的所有事件位）
		// 不用担心这里没事件位没复位，中途又新的重复事件出现，因为现在是CAN发送处理完了才会再次开启诊断，不会入大于出
		uxBits = xEventGroupWaitBits(
			MonitorAnalyzeEventHandle, /* 大电流HSD相关事件（ADC相对通道转换完成（4bit）、发生错误（1bit）、一组处理完成已发出数据（1bit））句柄 */
			BIT_6 | BIT_12 | BIT_17,   /* HSD1\HSD2\BAT完成一包数据处理,对应事件位BIT_20\BIT_21\BIT22(BIT从BIT_0开始);和故障出现事件 */
			pdTRUE,					   /* 事件标志位不复位后退出 */
			pdFALSE,				   /* 或关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);			   /* 无限等，否则直接读事件组返回（改为等无限久） */

		myprintf("can_app.c:into can data send,uxBits is %d\r\n", uxBits);
		if ((uxBits & BIT_6) != 0)
		{
			for (i = 0; i < 20; i++)
			{
				// HSD1一包数据入包完成
				if (xQueueReceive(AVDataQueueHandle,
								  &AV_Data[i], // 接收到指定位置
								  portMAX_DELAY) == pdFAIL)
					myprintf("can_app.c:CAN receive HSD1 data error\r\n");
			}
			// 告诉CAN发包函数我处理完了这小包
			flag1++;

			// 告诉HSD1诊断任务我处理完了，你可以继续了
			uxBits1 = xEventGroupSetBits(
				MonitorAnalyzeEventHandle, /* 因为MonitorAnalyzeEventHandle快满了临时占用CanReceiveEventHandle. */
				BIT_7);					   /* The bits being set. */
		}
		else if ((uxBits & BIT_12) != 0)
		{

			for (i = 0; i < 10; i++)
			{
				// HSD2一包数据入包完成
				if (xQueueReceive(AVDataQueueHandle,
								  &AV_Data[i + 20], // 接收到指定位置
								  portMAX_DELAY) == pdFAIL)
					myprintf("can_app.c:CAN receive HSD2 data error\r\n");
			}
			// 告诉CAN发包函数我处理完了这小包
			flag2++;

			// 告诉HSD2诊断任务我处理完了，你可以继续了
			uxBits1 = xEventGroupSetBits(
				MonitorAnalyzeEventHandle, /* 因为MonitorAnalyzeEventHandle快满了临时占用CanReceiveEventHandle. */
				BIT_13);				   /* The bits being set. */
		}
		else if ((uxBits & BIT_17) != 0)
		{
			//			if (flag1 == 0)
			//			{
			for (i = 0; i < 2; i++)
			{
				// BAT一包数据入包完成
				if (xQueueReceive(AVDataQueueHandle,
								  &AV_Data[i + 30], // 接收到指定位置
								  portMAX_DELAY) == pdFAIL)
					myprintf("can_app.c:CAN receive BAT data error\r\n");
			}
			// 告诉CAN发包函数我处理完了这小包
			flag3++;

			// 告诉HSD1诊断任务我处理完了，你可以继续了
			uxBits1 = xEventGroupSetBits(
				MonitorAnalyzeEventHandle, /* 因为MonitorAnalyzeEventHandle快满了临时占用CanReceiveEventHandle. */
				BIT_18);				   /* The bits being set. */
		}
		// HSD1\HSD2\BAT各都处理好了至少一包（放最后是为保证至少都处理了一遍）
		if ((flag1>100)|(flag2>100)|(flag3>100))
		{

			// CAN发送一包数据,且发送成功(发数据包ID：0x0F146A72)
			if (CAN_Transmit((const void *)AV_Data, 32, 0x0F146A72) == 1)
			{
				myprintf("can_app.c:can send a page of data\r\n");
			}
			flag1 = flag2 = flag3 = 0;
		}
		// 不要进入阻塞（回调里不要延时，还没处理完返回原处理，使数据收发不同步了）
		//		osDelay(1);
	}
}

void CAN_Error_Send_App(void)
{
	uint8_t i; // flag1:HSD1处理完成至少一包 flag2：HSD2处理好至少一包 flag3：BAT处理好至少一包
	uint8_t Error_Count_timely;
	uint16_t AV_Error_Data;
	EventBits_t uxBits; // 读取事件组的返回值（已配置事件组宽度为24bit）

	/* Infinite loop */
	for (;;)
	{
		// 故障码来(该任务的优先级一定要比OLED处理高，且不能有延时（属回调处理任务）)
		uxBits = xEventGroupWaitBits(
			MonitorAnalyzeEventHandle, /* 大电流HSD相关事件（ADC相对通道转换完成（4bit）、发生错误（1bit）、一组处理完成已发出数据（1bit））句柄 */
			BIT_23,					   /* HSD1\HSD2\BAT完成一包数据处理,对应事件位BIT_20\BIT_21\BIT22(BIT从BIT_0开始);和故障出现事件 */
			pdTRUE,					   /* 事件标志位不复位后退出 */
			pdFALSE,				   /* 或关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);			   /* 无限等，否则直接读事件组返回（改为等无限久） */

		//			// 分析一次事件组读取中故障事件个数(注意uxBits返回的值是整个事件组的值！因此要缩一下目标)
		//			uxBits = uxBits & (BIT_4 | BIT_5 | BIT_6 | BIT_7 | BIT_12 | BIT_13 | BIT_14 | BIT_15 | BIT_17 | BIT_18);
		//			Error_Count_timely = count_one_bits((unsigned int)uxBits);
		//			// 循环
		//			for (i = 0; i < Error_Count_timely; i++)

		// Peek a message on the created queue.  Block for 10 ticks if a message is not immediately available.
		// pcRxedMessage now points to the struct AMessage variable posted by vATask, but the item still remains on the queue.
		if (xQueueReceive(OLEDtoCANerrorSendHandle,
						  &AV_Error_Data,
						  portMAX_DELAY) == pdTRUE) // 要求CAN收错的优先级比OLED收错优先级要高（不然先被OLED剪切走了）portTICK_PERIOD_MS：实时  portMAX_DELAY：无期限
		{
			// CAN发送一帧错误,且如果发送成功(发错误帧ID：0x0F146B72)
			if (CAN_Transmit((const void *)&AV_Error_Data, 2, 0x0F146B72) == 1)
			{
				myprintf("can_app.c:can send a frame of error data\r\n");

				// 串口代CAN測試（串口打印代替CAN发送）
				//							for (i = 0; i < 32; i++)
				//								myprintf("%d\r\n", AV_Error_Data);
			}
		}
		//		osDelay(1);
	}
}

// CAN数据接收与解码，传出控制事件（可以考虑inline，将代码块嵌入到每个调用该函数的地方，虽减少了函数的调用，使代码执行的效力提高，但是会增加目标代码的大小，最终会使程序的代码段占有大量的内存）
void CAN_Recv_App(void)
{
	uint8_t i;
	uint8_t Command_Buffer[8];

	BaseType_t xReturn = pdPASS; /* 定义一个创建信息返回值，默认为pdPASS */
	EventBits_t uxBits;			 // 读取事件组的返回值（已配置事件组宽度为24bit）

	/* Infinite loop */
	for (;;)
	{
		// CAN接收中断回调里接收到数据
		uxBits = xEventGroupWaitBits(
			CanReceiveEventHandle,		   /* 大电流HSD相关事件（ADC相对通道转换完成（4bit）、发生错误（1bit）、一组处理完成已发出数据（1bit））句柄 */
			BIT_0 | BIT_1 , /* 指代VCU、仪表发来can控制指令的事件(设置了4位) */
			pdTRUE,						   /* 事件标志位复位后退出 */
			pdFALSE,					   /* 或关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);				   /* 等无限久 */

		//		myprintf("test event read %d\r\n",uxBits);

		//为VCU发来的报文
		if((uxBits&BIT_0)!=0)
		{
		// 数据接收、解码、唤起事件
		for (i = 0; i < 4; i++)
		{
			if (xQueueReceive(CanReceiveQueueHandle,
							  &Command_Buffer[i],
							  10) == pdFAIL) // portMAX_DELAY：无限延时
				myprintf("can_app.c:queue receive error\r\n");
		}
			//由HSD控制任务处理
			uxBits = xEventGroupSetBits( // 跳转至其他地方具体执行控制
						CanReceiveEventHandle,	 // The event group being updated.
						(const EventBits_t)Command_Buffer[0]<<8);					 // The bits being set.
			
			//在这里直接驱动PWM风扇(策略改了)
//			__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, Command_Buffer[1]); // 配置比较寄存器(相对于100的自动重装载值)
//					HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);		   // 背光开启，显形
//			
//			__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, Command_Buffer[2]); // 配置比较寄存器(相对于100的自动重装载值)
//					HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);		   // 背光开启，显形
			
			if(Command_Buffer[1]==100)
				HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, 1);		//FAN1_PWR_EN
			else 
				HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, 0);		//FAN1_PWR_EN
			
			if(Command_Buffer[2]==100)
				HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, 1);		//FAN2_PWR_EN
			else 
				HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, 0);		//FAN2_PWR_EN
			
			if(Command_Buffer[3]==100)
				HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, 1);		//FAN3_PWR_EN
			else 
				HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, 0);		//FAN3_PWR_EN
		}

		
		//仪表发的数据
//		else if()
//		{
//			
//		}
		
			// 回调处理任务，不延时
			//			osDelay(1);
	}
}


// CAN FIFO0 数据接收中断回调函数  hcan CAN句柄
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	uint8_t i;
	uint8_t RxData[8];			// CAN1数据接收缓存（未考虑8bit收不完）
	uint8_t static Counter = 0; // 帧计数器
	HAL_StatusTypeDef status;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult;
	BaseType_t xReturn = pdPASS; /* 定义一个创建信息返回值，默认为pdPASS */
	EventBits_t uxBits;			 // 读取事件组的返回值（已配置事件组宽度为24bit）

	if (hcan == &hcan1)
	{
		status = HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData); // 全接收返回是否get成功（接收ID和数据）（未考虑情况1帧收不完）
		if (HAL_OK == status)												  // CAN接收成功，开始干活
		{
			//接收来自VCU的报文（一包报文4字节，首字节为设备供电开关信号，后两字节为风扇PWM控制信号）
			if (RxHeader.ExtId == 0x00000202 )
			{
				for(i=0;i<4;i++)
				{
				// 分析接收数组接收数据量，数据全发送至队列
				xReturn = xQueueSendFromISR(CanReceiveQueueHandle,
											RxData, // 存放指向ADC2_Data地址的指针(问题：可一次把数组里多组数据发出？)
											0);
					xResult = xEventGroupSetBitsFromISR(
						CanReceiveEventHandle, /* The event group being updated. */
						BIT_0,				   /* The bits being set. */
						&xHigherPriorityTaskWoken);
				}
			}
			// 解析来自仪表的报文信号
			else if (RxHeader.ExtId == 0x1234FFFF)
			{

				}
			}
			// 不是我想要的，清空（或者不清空等被覆盖也行）
			else
				memset(&RxData, 0, 1);
	}
}

// 处理控制指令
//			memcpy(CAN1_TX_BUF, CAN1_RX_BUF, sizeof(CAN1_RX_BUF));		// 拷贝出数据
//			myprintf("CAN1_TX_BUF[%d]:0x%X\r\n", i, CAN1_TX_BUF[i]);

// 解码指令
//			device = Command_Buffer >> 4;
//			command = Command_Buffer << 4, command = command >> 4;
//switch (device)
//			{
//			case 0x0: // 电池箱风扇
//				if (command == 1)
//				{
//					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, 1);		//FAN3_PWR_EN
//				}
//				else if (command == 0)
//				{
//					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, 0);		//FAN3_PWR_EN
//				}
//				break;
//			case 0x1: // FAN1
//				if (command == 1)
//				{
//					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, 1);		//FAN1_PWR_EN
//				}
//				else if (command == 0)
//				{
//					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, 0);		//FAN1_PWR_EN
//				}
//				break;
//			case 0x2: // FAN2
//				if (command == 1)
//				{
//					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, 1);		//FAN2_PWR_EN
//				}
//				else if (command == 0)
//				{
//					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, 0);		//FAN2_PWR_EN
//				}
//				break;
//			case 0x3:			  // DRS
//				if (command == 2) // 开启
//				{
//					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 100); // 配置比较寄存器(相对于1000的自动重装载值)
//					HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);		   // 背光开启，显形
//				}
//				else if (command == 3) // 关闭
//				{
//					// 现在配置的PWM周期为0.02s，50Hz
//					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0); // 配置比较寄存器(相对于1000的自动重装载值)
//					HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);		 // 背光开启，显形
//				}
//				break;
//			case 0x4: // 尾灯
//				if (command == 1)
//				{
//					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, 1);		//TAILLIGHT_PWR_EN
//				}
//				else if (command == 0)
//				{
//					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, 0);		//TAILLIGHT_PWR_EN
//				}
//				break;
//			case 0x5: // 蜂鸣器
//				if (command == 1)
//				{
//					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 1);	//BUZZER_PWR_EN
//				}
//				else if (command == 0)
//				{
//					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 0);	//BUZZER_PWR_EN
//				}
//				break;
//			case 0x6: // 水泵
//				if (command == 1)
//				{
//					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, 1);		//PUMP_PWR_EN
//				}
//				else if (command == 0)
//				{
//					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, 0);		//PUMP_PWR_EN
//				}
//				break;
//			default:
//				myprintf("device no find\r\n");


				//CAN接收回调里只负责接收，使能标志位，处理在另外的CAN接收处理任务里
				//防止接收溢出（因中断的优先级一定比接收处理任务高），导致应该处理多次的只被接收处理任务当一次处理，留4事件位做CAN接收事件位，如果前位满，占后位 (引导到CAN接收处理的同时，取消进入休眠，暂未)(可能出现事件重复发生覆盖，导致未处理)
				//如果VCU发的控制报文频率不高，减少、取消事件位（因为不会有接收溢出）
//				switch ((xResult = xEventGroupGetBitsFromISR(CanReceiveEventHandle)) & (BIT_0 | BIT_1 | BIT_2 | BIT_3))
//				{
//				case 0:
//					xResult = xEventGroupSetBitsFromISR(
//						CanReceiveEventHandle, /* The event group being updated. */
//						BIT_0,				   /* The bits being set. */
//						&xHigherPriorityTaskWoken);
//					break;
//				case BIT_0:
//					xResult = xEventGroupSetBitsFromISR(
//						CanReceiveEventHandle, /* The event group being updated. */
//						BIT_1,				   /* The bits being set. */
//						&xHigherPriorityTaskWoken);
//					break;
//				case BIT_0 | BIT_1:
//					xResult = xEventGroupSetBitsFromISR(
//						CanReceiveEventHandle, /* The event group being updated. */
//						BIT_2,				   /* The bits being set. */
//						&xHigherPriorityTaskWoken);
//					break;
//				case BIT_0 | BIT_1 | BIT_2:
//					xResult = xEventGroupSetBitsFromISR(
//						CanReceiveEventHandle, /* The event group being updated. */
//						BIT_3,				   /* The bits being set. */
//						&xHigherPriorityTaskWoken);
//					break;
//				default: // CAN接收事件位溢出，或其他问题
//					myprintf("CAN receive overflow");
//				}


//// 数据解码与处理,根据接收事件位个数解码多少次
//		for (i = (uint8_t)count_one_bits(uxBits & (BIT_0 | BIT_1 | BIT_2 | BIT_3)); i > 0; i--)
//		{

#endif
