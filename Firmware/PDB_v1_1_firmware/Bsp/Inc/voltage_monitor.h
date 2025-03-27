#ifndef _VOLTAGE_DETECTION_H_
#define _VOLTAGE_DETECTION_H_

#include "main.h"
#include "stm32f4xx.h"

extern uint16_t ADC1_Data[2]; // 存放ADC1采集数据及处理后的电压值（实际电压*10），最后整合到AV_Data[32]数组中（要static?）

void Voltage_Detection(void);

#endif //_VOLTAGE_DETECTION_H_
