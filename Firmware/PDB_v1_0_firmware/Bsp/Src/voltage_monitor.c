
#if 1

#include "adc.h"
#include "voltage_monitor.h"
#include "myusart.h" //现在是耦合在里面地使用

uint16_t ADC1_Data[2]; // 存放ADC1采集数据及处理后的电压值（实际电压*10），最后整合到AV_Data[32]数组中（要static?）

void Voltage_Detection(void)
{
	HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC1_Data, 2);
}

// ADC2转换完成回调在HSD_drive.c文件里，负责将采集的值处理为电压实际值

#endif
