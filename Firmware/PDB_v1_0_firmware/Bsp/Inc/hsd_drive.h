
#ifndef __HSD_DRIVER_H__
#define __HSD_DRIVER_H__

#include "main.h"

// extern uint8_t AV_Data[32];	//电流电压缓存数组，前30个单元存储ADC采集的电流数据*10（保留小数点后一位）；后2单元存储ADC采集的电压数据-20（保留一位小数）

typedef struct
{
	uint8_t HSDx; // HSD编号

	uint8_t HSD_OUTx; // HSD输出使能引脚编号

	uint8_t HSD_DSELx; // HSD数字相对片选引脚编号

	uint8_t HSD_ISx; // HSD电流检测引脚编号

} HSD_HandleTypeDef;

// IO口映射
// 片选引脚映射（引脚-用电器）

// ADC电流读取引脚映射（引脚-用电器）

// HSD供电使能脚映射（引脚-用电器）
void HSD_Output_Enable_HSDs(uint8_t HSD_PinState);
void HSD_Output_Enable_HSDPinx(uint8_t HSDpin_number, uint8_t HSD_PinState);
void HSD_Diagnosis_Enable_HSDs(uint8_t HSD_Diagnosis_State);
void HSD_Start_Diagnosis_HSDs(uint8_t HSD_Pin);

#endif
