#ifndef _VOLTAGE_DETECTION_H_
#define _VOLTAGE_DETECTION_H_

#include "main.h"
#include "stm32f4xx.h"

extern uint16_t ADC1_Data[2]; // ���ADC1�ɼ����ݼ������ĵ�ѹֵ��ʵ�ʵ�ѹ*10����������ϵ�AV_Data[32]�����У�Ҫstatic?��

void Voltage_Detection(void);

#endif //_VOLTAGE_DETECTION_H_
