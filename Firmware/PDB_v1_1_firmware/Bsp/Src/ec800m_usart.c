//实现基于MQTT协议订阅与接收消息
#if 0

// bsp
#include "ec800m_usart.h"	//若要使能该BSP驱动，请失能myusart驱动文件（因为只有一个串口）
// app
#include "mqtt_app.h"
// freertos
#include "cmsis_os.h" //已引用FreeRTOS.h
#include "queue.h"
#include "stream_buffer.h"
#include "event_groups.h"
#include "semphr.h"

//ec800m初始化
void ec800m_init()
{
	
}

//向EC800M发送消息
void ec800m_app()
{
	
}

//接收EC800M消息



#endif