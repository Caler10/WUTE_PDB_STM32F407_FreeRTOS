// PA0��λ���ʼΪ�ж϶�ȡģʽ���������ʱ����Ϊ��������
#if 1

// bsp
#include "myusart.h"
#include "oled.h"
// app
#include "standby.h"
// freertos
#include "cmsis_os.h" //������FreeRTOS.h
#include "queue.h"
#include "stream_buffer.h"
#include "event_groups.h"
#include "semphr.h"

extern EventGroupHandle_t CanReceiveEventHandle;

// PA0 WKUPģʽ�£����������أ����ѣ���λִ�У�	//�������ѵ�Ƭ���ϵ��ʼ���������
void Wakeup_App(void)
{
	/* Infinite loop */
	for (;;)
	{
		osDelay(1000);
	}
}

// ǰ�ڣ�GPIO��ȡ���Ϊ�͵�ƽ�����óɻ���ģʽ�������ߣ����ڣ���PA0 GPIO�ж϶�ȡģʽ�¼������½��أ�����Ϊ����ģʽ��������
//Ȼ��ɾ���������񣨻�δ��
void Standby_App(void)
{
	UBaseType_t x = 0;
	BaseType_t xReturn = pdPASS; /* ����һ��������Ϣ����ֵ��Ĭ��ΪpdPASS */
	EventBits_t uxBits;			 // ��ȡ�¼���ķ���ֵ���������¼�����Ϊ24bit��

	// ��ʱ0.1���ȡ����ֹһ�ϵ�ʹ����ղ��˳���(���������ȼ����)
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
			CanReceiveEventHandle, /* С����HSD����¼���ADC���ͨ��ת����ɣ�4bit������������1bit����һ�鴦������ѷ������ݣ�1bit�������(HSD2����¼�λ���¼����[9��16λ]) */
			BIT_23,				   /* �¼���־λ��1��С���������ͨ��1�����ϣ�2��С���������ͨ��2������ */
			pdTRUE,				   /* �¼���־λ��λ���˳� */
			pdFALSE,			   /* ���ϵ�����ȶ���λ�˲Ŵ����¼������� */
			portMAX_DELAY);		   /* ����100ms������ֱ�Ӷ��¼��鷵�� */

		OLED_ShowStr(0, 7, (unsigned char *)"ready to standby", 1);
		
		// ��ʱ��������
		osDelay(5000);

		OLED_FullyClear();
		
		__HAL_RCC_PWR_CLK_ENABLE();					  // ʹ��PWRʱ��
		if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) // ��鲢���ϵͳ�Ƿ�Ӵ���ģʽ�ָ�
		{
			/* Clear Standby flag */
			__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
		}
		HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1); // ��������ʹ�õĻ���Դ:PWR_WAKEUP_PIN1 connected to PA.00
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);		   // ���������صĻ��ѱ�־
		HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);  // �������ӵ�PA.00��WakeUp Pin
		HAL_PWR_EnterSTANDBYMode();				   // �������ģʽ

		osDelay(5000);
	}
}

// ���µ����ʱ����������
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult; /* xHigherPriorityTaskWoken must be initialised to pdFALSE. */

//	// ȷ���µ磨GLVMS-Ϊ0V��
//	if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == 0)
//	{
		xResult = xEventGroupSetBitsFromISR(
			CanReceiveEventHandle, /* The event group being updated. */
			BIT_23,				   /* The bits being set. */
			&xHigherPriorityTaskWoken);
//	}
}

#endif