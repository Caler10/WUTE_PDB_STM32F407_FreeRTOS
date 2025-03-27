// hsd的基本驱动函数，供上层应用调用
#if 1

#include "gpio.h" //各引脚控制
#include "adc.h"  //有用到ADC DMA
#include "hsd_drive.h"
#include "myusart.h"

/*
 * 小电流HSD:
 * HSD诊断规则：DEN为0失能诊断，IS高阻抗；EN且被片选选中，开始对应检测	疑问：延时？
 * 诊断DSEL真值表：00-OUT0 01-OUT1 10-OUT2 11-OUT3
 * IS失效情况：与GND短路、限流、过压、动态温升过度或开路、负载关闭
 */

/*
引脚映射：
L1_select0 -> PE8
L1_select1 -> PE11
L1_5V_Backup_EN -> PB2!
L2_24V_Backup_EN -> PE7!
FIREON_PWR_EN -> PE9
DSBD_PWR_EN -> PE10
L1_current_IN3 -> PA3

L2_select0 -> PE13
L2_select1 -> PB10
L3_5V_Backup_EN -> PB1!
L1_24V_Backup_EN -> PE12
BSPD_PWR_EN -> PE14
ACCU_PWR_EN -> PE15!
L2_current_IN4 -> PA4

L3_select0 -> PB13
L3_select1 -> PD8
L2_5V_Backup_EN -> PB11!
Temperature_PWR_EN -> PB12
VCU_PWR_EN -> PB14
BDU_PWR_EN -> PB15!
L3_currrent_IN5 -> PA5

L4_select0 -> PD11
L4_select1 -> PA8
TAILLIGHT_PWR_EN -> PD9（默认关闭）!
TSALR_PWR_EN -> PD10!
BUZZER_PWR_EN -> PD12（默认关闭）
LOGGER_PWR_EN -> PD13
L4_current_IN6 -> PA6


L5_select0 -> PC6
L5_select1 -> PC9
L4_5V_Backup_EN -> PD14!
ENERGE_METER_PWR_EN -> PD15
ACCU_UBR_EN -> PC7
MCU_UBR_EN -> PC8
L5_current_IN7 -> PA7

H1_select -> PC12
VCU_UBR_EN -> PD0
IL+_PWR_EN -> PC11!
H1_current_IN10 -> PC0

H2_select -> PD2
H1_24V_Backup_EN -> PD3!
PUMP_PWR_EN -> PD1（默认关闭）
H2_current_IN11 -> PC1

H3_select -> PD5
H2_24V-Backup_EN -> PD6!
FAN1_PWR_EN -> PD4（默认关闭）
H3_current_IN12 -> PC2

H4_select -> PE2
FAN3_PWR_EN -> PE3（默认关闭）
FAN2_PWR_EN -> PE1（默认关闭）
H3_current_IN13 -> PC3

H5_select -> PE5
DRS_PWR_8V_EN -> PE6
H3_24V_Backup_EN -> PE4
H5_current_IN1 -> PA1

 */

// 为什么数组多1单元，留一单元告知相对HSD编号数（可换更好方法）
uint16_t ADC2_Data[5]; // 存放ADC2原始读取值和处理后的小电流HSD监测电流值（实际电流*10），最后压缩格式整合到AV_Data[32]数组中
uint16_t ADC3_Data[5]; // 存放ADC3原始读取值和处理后的大电流HSD监测电流值（实际电流*10），最后压缩格式整合到AV_Data[32]数组中
uint8_t now_HSD1_channelx;
uint8_t now_HSD2_channelx;
/*故障协议:
0x10:过流故障且已消除		0x11:过流故障未消除
0x20:..
*/

// 实现对所有HSD所有输出口输出使能、失能的控制
void HSD_Output_Enable_HSDs(uint8_t HSD_PinState)
{
	// 小电流HSD
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, HSD_PinState);	 	//L1_5V_Backup_EN
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, HSD_PinState);		//L2_24V_Backup_EN
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, HSD_PinState);		//FIREON_PWR_EN
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, HSD_PinState);	//DSBD_PWR_EN

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, HSD_PinState);		//L3_5V_Backup_EN
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, HSD_PinState);	//L1_24V_Backup_EN
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, HSD_PinState);	//BSPD_PWR_EN
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, HSD_PinState);	//ACCU_PWR_EN

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, HSD_PinState);	//L2_5V_Backup_EN
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, HSD_PinState);	//Temperature_PWR_EN
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, HSD_PinState);	//VCU_PWR_EN
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, HSD_PinState);	//BDU_PWR_EN

	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, HSD_PinState);		//TAILLIGHT_PWR_EN
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, HSD_PinState);	//TSALR_PWR_EN
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, HSD_PinState);	//BUZZER_PWR_EN
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, HSD_PinState);	//LOGGER_PWR_EN

	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, HSD_PinState);	//L4_5V_Backup_EN
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, HSD_PinState);	//ENERGE_METER_PWR_EN
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, HSD_PinState);		//ACCU_UBR_EN
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, HSD_PinState);		//MCU_UBR_EN

	// 大电流HSD
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, HSD_PinState);		//VCU_UBR_EN
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, HSD_PinState);	//IL+_PWR_EN

	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, HSD_PinState);		//H1_24V_Backup_EN
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, HSD_PinState);		//PUMP_PWR_EN

	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, HSD_PinState);		//H2_24V-Backup_EN
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, HSD_PinState);		//FAN1_PWR_EN

	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, HSD_PinState);		//FAN3_PWR_EN
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, HSD_PinState);		//FAN2_PWR_EN

	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, HSD_PinState);		//DRS_PWR_8V_EN
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, HSD_PinState);		//H3_24V_Backup_EN
}

// 实现对每个HSD的输出使能、失能的单独控制
// HSDpin_number：每个输出口在所有HSD输出口中的绝对编号（=HSDx+HSD_OUTx,HSDx:0x10~0xA0,HSD_OUTx:0x01~0x03）
void HSD_Output_Enable_HSDPinx(uint8_t HSDpin_number, uint8_t HSD_PinState)
{

	uint8_t HSDx, HSD_OUTx;

	HSDx = HSDpin_number, HSDx = HSDx >> 4; // 获得HSD编号，如果编号是0x0则表示对所有HSD操作

	HSD_OUTx = HSDpin_number << 4, HSD_OUTx = HSD_OUTx >> 4;

	if (HSDx <= 0x5)
	{
		switch (HSD_OUTx)
		{
		case 0x1:
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, HSD_PinState);	//
			break;
		case 0x2:
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, HSD_PinState);	//
			break;
		case 0x3:
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, HSD_PinState);	//
			break;
		case 0x4:
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, HSD_PinState);	//
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, HSD_PinState);	//
			break;
		default:
			myprintf("HSD output enable error");
		}
	}
	else if (HSDx > 0x5 || HSDx == 0x0)
	{
		HSD_OUTx = HSD_OUTx % 2; // 将HSD_OUTx由1、2、3、4变成1、2两枚举量
		switch (HSD_OUTx)
		{
		case 0x1:
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, HSD_PinState);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, HSD_PinState);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, HSD_PinState);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, HSD_PinState);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, HSD_PinState);
			break;
		case 0x2:
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, HSD_PinState);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, HSD_PinState);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, HSD_PinState);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, HSD_PinState);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, HSD_PinState);
			break;
		default:
			myprintf("HSD output enable error");
		}
	}
}

// 实现分大电流HSD和小电流HSD（或对所有HSD：0x00）同时地片选相同相对位置的输出口的电流，通过ADC+DMA将所有采集数据整理至数组，DMA完成回调负责分析故障返回相关数据（故障码）
// 不支持当HSDx==0x00时表示对所有HSD操作
void HSD_Start_Diagnosis_HSDs(uint8_t HSDpin_number)
{
	uint8_t HSDx, HSD_OUTx;

	HSDx = HSDpin_number, HSDx = HSDx >> 4; // 获得HSD编号，如果编号是0x0则表示对所有HSD操作

	HSD_OUTx = HSDpin_number << 4, HSD_OUTx = HSD_OUTx >> 4;

	//	myprintf("HSDx:%d HSD_OUTx:%d \r\n",HSDx,HSD_OUTx);

	if (HSDx <= 0x5) // 属于小电流HSD或对所有HSD
	{
		switch (HSD_OUTx)
		{
		case 0x1:
			// HSD1
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, 0);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, 0);
			// HSD2
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 0);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 0);
			// HSD3
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, 0);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, 0);
			// HSD4
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, 0);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0);
			// HSD5
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);
			break;
		case 0x2:

			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, 1);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, 0);

			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 1);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 0);

			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, 1);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, 0);

			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, 1);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0);

			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);
			break;
		case 0x3:

			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, 0);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, 1);

			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 0);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 1);

			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, 0);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, 1);

			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, 0);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 1);

			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 1);
			break;
		case 0x4:

			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, 1);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, 1);

			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, 1);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 1);

			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, 1);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, 1);

			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, 1);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 1);

			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 1);
			break;
		default:
			myprintf("HSD output enable error\r\n");
		}

		now_HSD1_channelx = HSD_OUTx; // 告知后面的处理过程相对HSD输出口编号
		HAL_ADC_Start_DMA(&hadc2, (uint32_t *)ADC2_Data, 5);
	}
	else if (HSDx > 0x5 )
	{
		//HSD_OUTx只能输入1，2
		switch (HSD_OUTx)
		{
		case 0x1:
			// HSD6
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, 0);
			// HSD7
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, 0);
			// HSD8
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, 0);
			// HSD9
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, 0);
			// HSD10
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, 0);
			break;
		case 0x2:
			// HSD6
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, 1);
			// HSD7
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, 1);
			// HSD8
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, 1);
			// HSD9
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, 1);
			// HSD10
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, 1);
			break;
		default:
			myprintf("HSD output enable error\r\n");
		}

		now_HSD2_channelx = HSD_OUTx; // 告知后面的处理过程相对HSD输出口编号
		HAL_ADC_Start_DMA(&hadc3, (uint32_t *)ADC3_Data, 5);
	}
}

// 可以考虑再做一个可以对单个输出口检测电流的函数

#endif
