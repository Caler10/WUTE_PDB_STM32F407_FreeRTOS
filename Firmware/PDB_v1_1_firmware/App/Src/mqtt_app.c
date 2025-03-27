//模块初始化，使其连接好服务器可以开始正常收发消息
#if 0

// bsp
#include "ec800m_usart.h"	//若要使能该BSP驱动，请失能myusart驱动文件
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

//ec800m收发消息
void ec800m_app()
{
	
}





#endif