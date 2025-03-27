// 控制OLED显示的上层应用（与其他驱动、上层应用解耦），通过信号量传递信息
// 还未实现错误码无限不重样累计错误计数（这里Error_Count现在最多8条（OLED最多显示6条），再多难保证不重样（不然复杂度很高））
#if 1

// bsp
#include "oled.h"
#include "myusart.h"
// app
#include "oled_app.h"
// freertos
#include "cmsis_os.h" //已引用FreeRTOS.h
#include "queue.h"
#include "stream_buffer.h"
#include "event_groups.h"
#include "semphr.h"

#define text_length 20 // 设置OLED一次显示字符串最长20

char Oled_First_Text[8] = "Error  0";				// 设计最多可显示99个错误
char Oled_Text_Buffer[text_length] = "HELLO WORLD"; // OLED字符串发送缓冲区

extern EventGroupHandle_t MonitorAnalyzeEventHandle;
extern SemaphoreHandle_t OLEDoccupiedSemHandle;
extern QueueHandle_t AVErrorQueueHandle;
extern QueueHandle_t OLEDtoCANerrorSendHandle;

void Oled_App(void)
{
	uint8_t i, j, before_row = 0;				 // same_flag已有相同故障，记录相同已显示故障的位置
	uint8_t Error_Count = 0, Error_Count_timely; // 错误情况计数，给OLED第一行显示
	uint16_t Error_Data = 0;					 // 用于临时存放传来的故障信息
	uint16_t OLED_Error_Buffer[32];				 // 最多存放8条报警
	EventBits_t uxBits;							 // 读取事件组的返回值（已配置事件组宽度为8bit）;my_uxBits
	EventBits_t uxBitsToSet;
	BaseType_t semReturn = pdPASS, queueReturn = pdPASS; /* 在这里做接收信号量放回值 */

	for (i = 0; i < 32; i++)
	{
		OLED_Error_Buffer[i] = 0xf0ff;
	}
	/* Infinite loop */
	for (;;)
	{
		// 出现故障事件（最后不用再清除故障事件了，这里收到后已经清理了一次;另外，uxBits是返回事件组里的所有事件位）
		uxBits = xEventGroupWaitBits(
			MonitorAnalyzeEventHandle, /* 大电流HSD相关事件（ADC相对通道转换完成（4bit）、发生错误（1bit）、一组处理完成已发出数据（1bit））句柄 */
			BIT_4 | BIT_10 | BIT_15,   /* HSD1、HSD2、BAT错误发生事件 */
			pdTRUE,					   /* 事件标志位复位后退出 */
			pdFALSE,				   /* 或关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);			   /* 等无限久 */

		uxBitsToSet = 0;
		if ((uxBits & BIT_4) == BIT_4)
		{
			/* xEventGroupWaitBits() returned because both bits were set. */
			uxBitsToSet |= BIT_5;
		}
		else if ((uxBits & BIT_10) != 0)
		{
			/* xEventGroupWaitBits() returned because just BIT_0 was set. */
			uxBitsToSet |= BIT_11;
		}
		else if ((uxBits & BIT_15) != 0)
		{
			/* xEventGroupWaitBits() returned because just BIT_4 was set. */
			uxBitsToSet |= BIT_16;
		}

		// 集中处理：分析这次事件组读取中故障事件个数
		uxBits = uxBits & (BIT_4 | BIT_10 | BIT_15);
		Error_Count_timely = count_one_bits((unsigned int)uxBits);

//		myprintf("monitor error number timely is %d\r\n", Error_Count_timely);

		for (i = 0; i < Error_Count_timely; i++)
		{
			// 重置
			before_row = 0;
			// 接收故障队列
			if (xQueueReceive(AVErrorQueueHandle,
							  &Error_Data,
							  portMAX_DELAY) == pdFAIL) // portTICK_PERIOD_MS：实时  portMAX_DELAY：无期限
				myprintf("AVErrorQueue Receive error\r\n");

			// 与之前已显示的32条报警比较，同则替换
			for (j = 0; j < 32; j++) // 嵌套for不能再用i!!
			{
				// 之前有出现过相同位置的故障
				if ((Error_Data >> 8) == (OLED_Error_Buffer[j] >> 8)) // 会不会导致对变量运算了（？）
				{
					// 索引到之前有出现过
					before_row = j + 1;

					// OLED能显示才给
					if (before_row < 7)
					{
						// strcpy((char*)Oled_Text_Buffer, "hello");	//不行(OLED字符显示输入数字显示的是ASCIS码对应内容)
						Int2String(Error_Data, Oled_Text_Buffer, 16); // 16进制
						// 填入旧OLED显示位置
//						OLED_ShowStr(10, before_row, (unsigned char *)Oled_Text_Buffer, 1); // xReturn-1？

//						myprintf("before_row %d\r\n", before_row);
					}
				}
			}

			// 获取错误码计数
			semReturn = uxSemaphoreGetCount(OLEDoccupiedSemHandle);
			myprintf("OLED semaphore count is %d\r\n", semReturn);
			// 新错误，填充OLED新的区域

			if (before_row == 0) // 说明之前没出现过
			{
				if (semReturn > 26) // get OLED占用信号量当前计数（初始为32）,是否已使用6个，就不填充屏幕了
				{
					// strcpy((char*)Oled_Text_Buffer, "%d",Error_Data);	//不行
					Int2String(Error_Data, Oled_Text_Buffer, 16);							  // 16进制
//					OLED_ShowStr(10, (33 - semReturn), (unsigned char *)Oled_Text_Buffer, 1); // Oled_Text_Buffer内必须字符格式，否则被当ascic码
				}
				// pdFALSE	OLED显示资源无了
				else
					myprintf("the oled is occupied all\r\n");

				// 同时发送给CAN发给整车(这里xReturn和上面信号量返回复用了（问题原因）)
				queueReturn = xQueueSend(OLEDtoCANerrorSendHandle,
										 &Error_Data,
										 0);
				uxBits = xEventGroupSetBits(
					MonitorAnalyzeEventHandle, /* 因为MonitorAnalyzeEventHandle快满了临时占用CanReceiveEventHandle. */
					BIT_23);				   /* The bits being set. */

				// 记录在案
				OLED_Error_Buffer[32 - semReturn] = Error_Data;

				semReturn--; // 错误码计数减少！
				//					myprintf("Oled occup");

				// OLED显示资源占用减1（不擦除故障信息，即信号量不减少）
				if (xSemaphoreTake(OLEDoccupiedSemHandle, (TickType_t)0) == pdTRUE)
				{
					//					myprintf("OLED occupied semaphore add 1\r\n");
				}
			}

			Error_Count = 32 - semReturn;
//			myprintf("Error_Count:%d\r\n", Error_Count);
			// OLED首行错误码显示计数加一
			if (Error_Count < 10)
				// Oled_First_Text[7] = Error_Count + 48 + 1; // 需要赋值字符‘'，选择ascii码操作(会溢出？显示有bug)
				Int2String(Error_Count, &Oled_First_Text[7], 10); // 显示1~9的数有bug，成10~90
			else
				Int2String(Error_Count, &Oled_First_Text[6], 10); // 显示1~9的数有bug，会成10~90
//			OLED_ShowStr(0, 0, (unsigned char *)Oled_First_Text, 1);
		}

		uxBits = xEventGroupSetBits(
			MonitorAnalyzeEventHandle, /* 因为MonitorAnalyzeEventHandle快满了临时占用CanReceiveEventHandle. */
			uxBitsToSet);			   /* The bits being set. */

		//		myprintf("uxBitsToSet %d\r\n", uxBitsToSet);
		//		osDelay(1);
	}
}

//？进制数转字符串函数
char* Int2String(int num,char* str,int radix) 
{
    char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";//索引表
    unsigned unum;//存放要转换的整数的绝对值,转换的整数可能是负数
    int i=0,j,k;//i用来指示设置字符串相应位，转换之后i其实就是字符串的长度；转换后顺序是逆序的，有正负的情况，k用来指示调整顺序的开始位置;j用来指示调整顺序时的交换。
 
    //获取要转换的整数的绝对值
    if(radix==10&&num<0)//要转换成十进制数并且是负数
    {
        unum=(unsigned)-num;//将num的绝对值赋给unum
        str[i++]='-';//在字符串最前面设置为'-'号，并且索引加1
    }
    else unum=(unsigned)num;//若是num为正，直接赋值给unum
 
    //转换部分，注意转换后是逆序的
    do
    {
        str[i++]=index[unum%(unsigned)radix];//取unum的最后一位，并设置为str对应位，指示索引加1
        unum/=radix;//unum去掉最后一位
 
    }while(unum);//直至unum为0退出循环
 
    str[i]='\0';//在字符串最后添加'\0'字符，c语言字符串以'\0'结束。
 
    //将顺序调整过来
    if(str[0]=='-') k=1;//如果是负数，符号不用调整，从符号后面开始调整
    else k=0;//不是负数，全部都要调整
 
    char temp;//临时变量，交换两个值时用到
    for(j=k;j<=(i-1)/2;j++)//头尾一一对称交换，i其实就是字符串的长度，索引最大值比长度少1
    {
        temp=str[j];//头部赋值给临时变量
        str[j]=str[i-1+k-j];//尾部赋值给头部
        str[i-1+k-j]=temp;//将临时变量的值(其实就是之前的头部值)赋给尾部
    }
 
    return str;//返回转换后的字符串
}

#endif
