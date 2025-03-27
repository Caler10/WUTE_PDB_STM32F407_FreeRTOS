// PA0复位后最开始为中断读取模式，后进休眠时配置为唤醒引脚
#if 1

// bsp
#include "myusart.h"
#include "oled.h"
// app
#include "standby.h"
// freertos
#include "cmsis_os.h" //已引用FreeRTOS.h
#include "queue.h"
#include "stream_buffer.h"
#include "event_groups.h"
#include "semphr.h"

extern EventGroupHandle_t CanReceiveEventHandle;

// PA0 WKUP模式下，出现上升沿，唤醒（复位执行）	//后面打算把单片机上电初始化代码放这
void Wakeup_App(void)
{
	/* Infinite loop */
	for (;;)
	{
		osDelay(1000);
	}
}

// 前期：GPIO读取如果为低电平，配置成唤醒模式，进休眠；后期：在PA0 GPIO中断读取模式下检测出现下降沿，配置为唤醒模式，进休眠
//然后删除苏醒任务（还未）
void Standby_App(void)
{
	UBaseType_t x = 0;
	BaseType_t xReturn = pdPASS; /* 定义一个创建信息返回值，默认为pdPASS */
	EventBits_t uxBits;			 // 读取事件组的返回值（已配置事件组宽度为24bit）

	// 延时0.1秒读取（防止一上电就待机烧不了程序）(该任务优先级最高)
	osDelay(100);
	if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == 0)
	{
		uxBits = xEventGroupSetBits(
			CanReceiveEventHandle, /* The event group being updated. */
			BIT_23);			   /* The bits being set. */
	}
	/* Infinite loop */
	for (;;)
	{
		uxBits = xEventGroupWaitBits(
			CanReceiveEventHandle, /* 小电流HSD相关事件（ADC相对通道转换完成（4bit）、发生错误（1bit）、一组处理完成已发出数据（1bit））句柄(HSD2相关事件位于事件组的[9，16位]) */
			BIT_23,				   /* 事件标志位：1：小电流的相对通道1完成诊断；2：小电流的相对通道2完成诊断 */
			pdTRUE,				   /* 事件标志位复位后退出 */
			pdFALSE,			   /* 或关系（不等都置位了才代表事件发生） */
			portMAX_DELAY);		   /* 最多等100ms，否则直接读事件组返回 */

		OLED_ShowStr(0, 7, (unsigned char *)"ready to standby", 1);
		
		// 延时进入休眠
		osDelay(5000);

		OLED_FullyClear();
		
		__HAL_RCC_PWR_CLK_ENABLE();					  // 使能PWR时钟
		if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) // 检查并检查系统是否从待机模式恢复
		{
			/* Clear Standby flag */
			__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
		}
		HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1); // 禁用所有使用的唤醒源:PWR_WAKEUP_PIN1 connected to PA.00
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);		   // 清除所有相关的唤醒标志
		HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);  // 启用连接到PA.00的WakeUp Pin
		HAL_PWR_EnterSTANDBYMode();				   // 进入待机模式

		osDelay(5000);
	}
}

// 当下电操作时，触发休眠
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult; /* xHigherPriorityTaskWoken must be initialised to pdFALSE. */

//	// 确凿下电（GLVMS-为0V）
//	if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == 0)
//	{
		xResult = xEventGroupSetBitsFromISR(
			CanReceiveEventHandle, /* The event group being updated. */
			BIT_23,				   /* The bits being set. */
			&xHigherPriorityTaskWoken);
//	}
}

#endif