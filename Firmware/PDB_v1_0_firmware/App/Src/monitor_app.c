// 对所有hsd统一操作的上层应用
#if 1

// mx
#include "adc.h"
// bsp
#include "myusart.h"
#include "hsd_drive.h"
#include "voltage_monitor.h"
// app
#include "monitor_app.h"
// freertos
#include "cmsis_os.h" //已引用FreeRTOS.h
#include "queue.h"
#include "stream_buffer.h"
#include "event_groups.h"
#include "semphr.h"

// hsd_drive.c中声明
extern uint8_t now_HSD1_channelx;
extern uint8_t now_HSD2_channelx;
extern uint16_t ADC2_Data[5]; //(通过全局变量传参)存放ADC2原始读取值和处理后的小电流HSD监测电流值（实际电流*10），最后压缩格式整合到AV_Data_zip[32]数组中
extern uint16_t ADC3_Data[5]; // 存放ADC3原始读取值和处理后的大电流HSD监测电流值（实际电流*10），最后压缩格式整合到AV_Data_zip[32]数组中
// voltage_monitor.h中声明
extern uint16_t ADC1_Data[2]; // 存放ADC1采集数据及处理后的电压值（实际电压*10），最后整合到AV_Data[32]数组中

extern osEventFlagsId_t MonitorAnalyzeEventHandle;
extern osMessageQueueId_t AVErrorQueueHandle;
extern osMessageQueueId_t AVDataQueueHandle;

void HSD1_Monitor_App(void)
{
	uint8_t i;
	uint8_t aHSDx = 0, aHSD_OUTx = 0; // HSD_OUTx:相对诊断引脚编号；HSDx：HSD轮询计算、分析(为什么前面加a，因为是从0始，为区分)
	uint16_t HSD1_Error_Data;		  //[8,15]位指代HSD电池绝对通道编号，[4,7]位代错误码，[0，3]代是否错误;
	uint8_t ADC2_Data_zip[5][4];	  // 存储所有通道一包数据

	UBaseType_t x = 0;
	BaseType_t xReturn = pdPASS; /* 定义一个创建信息返回值，默认为pdPASS */
	EventBits_t uxBits;			 // 读取事件组的返回值（已配置事件组宽度为24bit）

	/* Infinite loop */
	for (;;)
	{
		// 开启小电流HSD逐通道诊断（ADC DMA）
		HSD_Start_Diagnosis_HSDs(0x10 + aHSD_OUTx + 1); // 对小电流HSD逐诊

		// 逐通道诊断完成（ADC DMA）
		uxBits = xEventGroupWaitBits(
			MonitorAnalyzeEventHandle, /* 小电流HSD相关事件（ADC相对通道转换完成（4bit）、发生错误（1bit）、一组处理完成已发出数据（1bit））句柄(HSD2相关事件位于事件组的[9，16位]) */
			1 << aHSD_OUTx,			   /* 事件标志位：1：小电流的相对通道1完成诊断；2：小电流的相对通道2完成诊断 */
			pdTRUE,					   /* 事件标志位复位后退出 */
			pdFALSE,				   /* 或关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);			   /* 最多等100ms，否则直接读事件组返回 */

		// 逐HSD的采集数据计算、检查、故障处理（跳转回调事件）
		for (aHSDx = 0; aHSDx < 5; aHSDx++)
		{
			// 计算：ADC电压值：V=x/4096*3.3	通道输出电流值*10：Il*10=V/650Ω*300（放大倍率）*10  避免浮点运算
			ADC2_Data_zip[aHSDx][aHSD_OUTx] = (uint32_t)ADC2_Data[aHSDx] * 37 / 10000; // 取0.0037简化值导致的误差=（0.0037184495192308-0.0037）*4096/10=0.007557A；原应0.0037184495192308

			// 检查：如果超出阈值
			if (ADC2_Data_zip[aHSDx][aHSD_OUTx] >= 8)
			{
				// 故障处理：填充HSD故障信息（高8位输出口绝对编号；故障码、故障状态是否恢复）
				HSD1_Error_Data = ((((aHSDx + 1) << 4) + aHSD_OUTx + 1) << 8) | 0x11;

				// 故障处理：压入队列（OLED读取到就擦除，CAN读不擦，但优先级更高，该任务优先级更更低），队列发送给OLED指示、CAN马上单条发出
				xReturn = xQueueSend(AVErrorQueueHandle,
									 &HSD1_Error_Data,
									 portTICK_PERIOD_MS);
				// 故障处理：唤起事件：指示去读故障队列
				uxBits = xEventGroupSetBits(
					MonitorAnalyzeEventHandle, /* The event group being updated. */
					BIT_4);					   /* 唤起对应的故障位（详情在main.h） */

				// 故障处理：等待事件：故障处理完了才继续
				uxBits = xEventGroupWaitBits(
					MonitorAnalyzeEventHandle, /* 因为MonitorAnalyzeEventHandle快满了临时占用CanReceiveEventHandle. */
					BIT_5,					   /* BIT_18:一包数据处理完了|BIT_21：故障相关处理完了 */
					pdTRUE,					   /* 事件标志位复位后退出 */
					pdTRUE,					   /* 与关系（不等都置位了才代表事件发生） */
					portMAX_DELAY);			   /* 最多等100ms，否则直接读事件组返回 */
			}
		}

		// 切换读取下一相对通道
		aHSD_OUTx++;

		// 数据包处理（当小电流HSD所有通道都get、计算好数据）
		if (aHSD_OUTx == 4) // 上面已经处理了4次(4个通道都过了一遍)
		{
			for (aHSDx = 0; aHSDx < 5; aHSDx++)
			{
				for (i = 0; i < 4; i++) // i代aHSD_OUTx（否则下面aHSD_OUTx重置开始有问题）
				{
					// 5个诊断数据队列发送给CAN，唤起事件
					xReturn = xQueueSend(AVDataQueueHandle,
										 &ADC2_Data_zip[aHSDx][i], // 存放指向ADC2_Data地址的指针(问题：可一次把数组里多组数据发出？)
										 0);
				}
			}
			// TEST:测试队列一次是发送多少消息（结论：引用一次，发送一只）
			//			myprintf("queue have %d\r\n",x);
			//			//x = uxQueueSpacesAvailable( AVDataQueueHandle );
			//			x = uxQueueMessagesWaiting( AVDataQueueHandle );
			//			myprintf("queue have used %d\r\n",x);

			// 唤起数据包CAN发送
			uxBits = xEventGroupSetBits(
				MonitorAnalyzeEventHandle, /* The event group being updated. */
				BIT_6);					   /* 设置BIT_21为HSD1一包数据处理完成标志位 */
//			myprintf("monitor_app.c:HSD1 a page of data handled\r\n");

			// 等数据包CAN发送完了才进行下一步
			uxBits = xEventGroupWaitBits(
				MonitorAnalyzeEventHandle, /* 因为MonitorAnalyzeEventHandle快满了临时占用CanReceiveEventHandle. */
				BIT_7,					   /* BIT_19:一包数据处理完了 */
				pdTRUE,					   /* 事件标志位复位后退出 */
				pdTRUE,					   /* 与关系（不等都置位了才代表事件发生） */
				portMAX_DELAY);			   /* 最多等100ms，否则直接读事件组返回 */

			// 重置开始
			aHSD_OUTx = 0;
		}
		osDelay(10);
	}
}

void HSD2_Monitor_App(void)
{
	uint8_t i;
	uint8_t aHSDx = 0, aHSD_OUTx = 0; // HSD_OUTx:相对诊断引脚编号；HSDx：HSD轮询计算、分析(为什么前面加a，因为是从0始，为区分)
	uint16_t HSD3_Error_Data;		  //[8,15]位指代HSD电池绝对通道编号，[4,7]位代错误码，[0，3]代是否错误;
	uint8_t ADC3_Data_zip[5][2];

	BaseType_t xReturn = pdPASS; /* 定义一个创建信息返回值，默认为pdPASS */
	EventBits_t uxBits;			 // 读取事件组的返回值（已配置事件组宽度为24bit）

	/* Infinite loop */
	for (;;)
	{
		HSD_Start_Diagnosis_HSDs(0x60 + aHSD_OUTx + 1); // 对大电流HSD逐诊

		// ADC DMA完成，中断回调触发了标志位
		uxBits = xEventGroupWaitBits(
			MonitorAnalyzeEventHandle, /* 大电流HSD相关事件（ADC相对通道转换完成（4bit）、发生错误（1bit）、一组处理完成已发出数据（1bit））句柄 */
			1 << (aHSD_OUTx + 8),	   /* 事件标志位：1：小电流的相对通道1完成诊断；2：小电流的相对通道2完成诊断(HSD2相关事件位于事件组的[9，16位]) */
			pdTRUE,					   /* 事件标志位复位后退出 */
			pdFALSE,				   /* 或关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);			   /* 最多等100ms，否则直接读事件组返回（改为等无限久） */

		// 对大电流HSD数据处理
		for (aHSDx = 0; aHSDx < 5; aHSDx++)
		{
			// 对大电流HSD诊断数据处理，得实际电流值*10
			// 计算：ADC电压值：V=x/4096*3.3	通道输出电流值*10：Il*10=V/650Ω*1450（放大倍率）*10  避免浮点运算
			ADC3_Data_zip[aHSDx][aHSD_OUTx] = (uint32_t)ADC3_Data[aHSDx] * 18 / 1000; // 同上HSD1

			// 如果有一通道有故障
			if (ADC3_Data_zip[aHSDx][aHSD_OUTx] >= 30)
			{
				// 填充HSD故障信息，包括输出口绝对编号、故障码、故障状态是否恢复
				HSD3_Error_Data = ((((aHSDx + 6) << 4) + aHSD_OUTx + 1) << 8) | 0x11;

//				myprintf("HSD3_Error_Data %d ADC3_Data_zip %d");
				// 发送给队列（OLED读取到就擦除，CAN读不擦，但优先级更高，该任务优先级更更低），队列发送给OLED指示、CAN马上单条发出
				xReturn = xQueueSend(AVErrorQueueHandle,
									 &HSD3_Error_Data,
									 0);
				// 唤起故障事件，指示OLED、CAN去处理故障队列
				uxBits = xEventGroupSetBits(
					MonitorAnalyzeEventHandle, /* The event group being updated. */
					BIT_10);				   /* 唤起对应的故障位（详情在main.h） */

				// 等待故障处理完了才继续
				uxBits = xEventGroupWaitBits(
					MonitorAnalyzeEventHandle, /* 因为MonitorAnalyzeEventHandle快满了临时占用CanReceiveEventHandle. */
					BIT_11,					   /* BIT_19:一包数据处理完了|BIT_22：故障相关处理完了 */
					pdTRUE,					   /* 事件标志位复位后退出 */
					pdTRUE,					   /* 与关系（不等都置位了才代表事件发生） */
					portMAX_DELAY);			   /* 最多等100ms，否则直接读事件组返回 */
			}
		}

		// 切换读取下一相对通道
		aHSD_OUTx++;

		// 如果已读2次，发送HSD1所有通道诊断数据后重置开始
		if (aHSD_OUTx == 2) // 上面已经处理了2次，得到所有HSD2通道数据
		{
			for (aHSDx = 0; aHSDx < 5; aHSDx++)
			{
				for (i = 0; i < 2; i++) // i代aHSD_OUTx（否则下面aHSD_OUTx重置开始有问题）
				{
					// 5*2个诊断数据队列发送给CAN，唤起事件
					xReturn = xQueueSend(AVDataQueueHandle,
										 &ADC3_Data_zip[aHSDx][aHSD_OUTx],
										 0);
				}
			}
			// 一包数据处理完成事件
			uxBits = xEventGroupSetBits(
				MonitorAnalyzeEventHandle, /* The event group being updated. */
				BIT_12);				   /* 设置BIT_22为HSD2一包数据处理完成标志位 */
//			myprintf("monitor_app.c:HSD2 a page of data handled\r\n");

			// 等CAN data send 接收完了事件来了数据才进行下一步
			uxBits = xEventGroupWaitBits(
				MonitorAnalyzeEventHandle, /* 因为MonitorAnalyzeEventHandle快满了临时占用CanReceiveEventHandle. */
				BIT_13,					   /* BIT_19:一包数据处理完了 */
				pdTRUE,					   /* 事件标志位复位后退出 */
				pdTRUE,					   /* 与关系（不等都置位了才代表事件发生） */
				portMAX_DELAY);			   /* 最多等100ms，否则直接读事件组返回 */

			// 重置开始
			aHSD_OUTx = 0;
		}
		osDelay(10);
	}
}

void BAT_Monitor_App(void)
{
	uint8_t aBATx = 0;		 // BATx:电池编号(为什么前面加a，因为是从0始，为区分)
	uint16_t BAT_Error_Data; //[8,15]位指代HSD电池绝对通道编号，[4,7]位代错误码，[0，3]代是否错误;
	uint8_t ADC1_Data_zip[2];

	BaseType_t xReturn = pdPASS; /* 定义一个创建信息返回值，默认为pdPASS */
	EventBits_t uxBits;			 // 读取事件组的返回值（已配置事件组宽度为24bit）

	/* Infinite loop */
	for (;;)
	{
		Voltage_Detection(); // 对电压检测（BAT1\BAT2电压分开监测，因为BAT1\2同时发生故障时，BAT1故障事件马上又BAT2故障事件，OLED\CAN来不及处理只处理了一次故障）

		// ADC DMA完成，中断回调触发了标志位(BAT1\BAT2数据都来了，所以下面两都处理)
		uxBits = xEventGroupWaitBits(
			MonitorAnalyzeEventHandle, /* BAT相关事件（ADC转换完成(1bit)、出现故障(1bit)、一组数据都处理完成(1bit)）（BAT相关事件位于[17，20]位） */
			BIT_14,					   /* 事件组第16位做ADC转换完成标志位 */
			pdTRUE,					   /* 事件标志位复位后退出 */
			pdFALSE,				   /* 或关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);			   /* 一直等 */

		for (aBATx = 0; aBATx < 2; aBATx++)
		{
			// 得实际电压值*10
			// 计算：ADC电压值：V=x/4096*3.3	通道输出电压值*10=x*0.008056640625,取0.008（偏差：0.0232001536V）
			ADC1_Data_zip[aBATx] = ADC1_Data[aBATx] * 8 / 1000; // 根据原理图

			// 如果出现故障
			if (ADC1_Data_zip[aBATx] >= 140)
			{
				// 填充HSD故障信息，包括输出口绝对编号、故障码、故障状态是否恢复
				BAT_Error_Data = ((0xB0 + (aBATx + 1)) << 8) | 0x21; // 例：0xB121:第一个电池发生过压故障，故障还存在
//				myprintf("monitor_app.c:BAT %d error\r\n", aBATx);

				// 错误码发送给队列（OLED读取到就擦除，CAN读不擦，但优先级更高，该任务优先级更更低），队列发送给OLED指示、CAN马上单条发出
				xReturn = xQueueSend(AVErrorQueueHandle,
									 &BAT_Error_Data,
									 0);
				// 唤起故障事件，指示去读故障队列
				uxBits = xEventGroupSetBits(
					MonitorAnalyzeEventHandle, /* The event group being updated. */
					BIT_15 << aBATx);		   /* The bits being set. */

				// 等待故障处理完了才继续
				uxBits = xEventGroupWaitBits(
					MonitorAnalyzeEventHandle, /* 因为MonitorAnalyzeEventHandle快满了临时占用CanReceiveEventHandle. */
					BIT_16,					   /* BIT_20:一包数据处理完了|BIT_23：故障相关处理完了 */
					pdTRUE,					   /* 事件标志位复位后退出 */
					pdTRUE,					   /* 与关系（不等都置位了才代表事件发生） */
					portMAX_DELAY);			   /* 最多等100ms，否则直接读事件组返回 */

				// OLED_ShowStr(0, BATx, (unsigned char*)ADC1_Data, 1);
//				myprintf("monitor_app.c:BAT %d V :%d \r\n", aBATx, ADC1_Data_zip[aBATx]);
			}
		}

		// 上面已经处理了2次，发送HSD1所有通道诊断数据后重置开始
		for (aBATx = 0; aBATx < 2; aBATx++)
		{
			// 2个诊断数据队列发送给CAN，唤起事件
			xReturn = xQueueSend(AVDataQueueHandle,
								 &ADC1_Data_zip[aBATx],
								 0);
			if (xReturn == errQUEUE_FULL)
				myprintf("monitor_app.c:AVdata queue full\r\n");
		}

		// 唤起事件
		uxBits = xEventGroupSetBits(
			MonitorAnalyzeEventHandle, /* The event group being updated. */
			BIT_17);				   /* The bits being set. */

		// 等CAN data send 接收完了事件来了数据才进行下一步
		uxBits = xEventGroupWaitBits(
			MonitorAnalyzeEventHandle, /* 因为MonitorAnalyzeEventHandle快满了临时占用CanReceiveEventHandle. */
			BIT_18,					   /* BIT_19:一包数据处理完了 */
			pdTRUE,					   /* 事件标志位复位后退出 */
			pdTRUE,					   /* 与关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);			   /* 最多等100ms，否则直接读事件组返回 */

		osDelay(10);
	}
}

// ADC转换回调函数，负责数据分析（实际电流、故障）、压缩（uint8_t格式），信息转发给CAN、OLED，（考虑只在回调里翻转标志位，由回调外函数执行具体任务？）
// ADC2_Data\ADC3_Data数据类型可再变小？
// 现在是ADC转换原始数据直接通过全局变量传递去处理，不通过队列了（问题，那边还没处理完这边就又更新了（不会发生此问题，因为APP流程是ADC数据处理完了才会再次开启检测，也可考虑使用信号量使更稳健））
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	uint8_t i = 0;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult; /* xHigherPriorityTaskWoken must be initialised to pdFALSE. */

	// 对小电流HSD诊断数据处理，得实际电流值*10
	// 计算：ADC电压值：V=x/4096*3.3	通道输出电流值*10：Il*10=V/650Ω*300（放大倍率）*10
	if (hadc == &hadc2)
	{
		HAL_ADC_Stop_DMA(&hadc2);
		// HSD1的ADC转换收到数据事件
		xResult = xEventGroupSetBitsFromISR(
			MonitorAnalyzeEventHandle,	  /* The event group being updated. */
			1 << (now_HSD1_channelx - 1), /* The bits being set. */
			&xHigherPriorityTaskWoken);
	}

	// 对大电流HSD诊断数据处理，得实际电流值*10
	// 计算：ADC电压值：V=x/4096*3.3	通道输出电流值*10：Il*10=V/650Ω*1450（放大倍率）*10
	else if (hadc == &hadc3)
	{
		HAL_ADC_Stop_DMA(&hadc3);
		// HSD2的ADC转换收到数据事件
		xResult = xEventGroupSetBitsFromISR(
			MonitorAnalyzeEventHandle,		  /* The event group being updated. */
			1 << (now_HSD2_channelx - 1 + 8), /* The bits being set. */
			&xHigherPriorityTaskWoken);
	}
	else if (hadc == &hadc1)
	{
		HAL_ADC_Stop_DMA(&hadc1);
		// BAT的ADC转换收到数据事件
		xResult = xEventGroupSetBitsFromISR(
			MonitorAnalyzeEventHandle, /* The event group being updated. */
			BIT_14,					   /* The bits being set. */
			&xHigherPriorityTaskWoken);
	}
}

// ADC转换失败回调函数
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
	myprintf("ADC error\r\n");
}

#endif
