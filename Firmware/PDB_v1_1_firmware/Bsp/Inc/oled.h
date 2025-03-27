#ifndef __OLED_H__
#define __OLED_H__

#include "stm32f4xx_hal.h" //链接HAL库

/*
	BMP图片声明
	图片格式为二位数组，下标分别对应图片的宽和高：
	BMP_xx[H/8][L];（就是你想显示一个图形，就要先声明一下，然后调用OLED_DrawBMP）
**************************************/
extern const uint8_t BMP_Picture[32 / 8][32];

/* 设置坐标点的状态 */
typedef enum
{
	SET_PIXEL = 0x01,
	RESET_PIXEL = 0x00,
} PixelStatus;

/* 功能函数声明 */
// 写数据，硬件IIC使用
void HAL_I2C_WriteByte(uint8_t addr, uint8_t data);
// 写命令
void WriteCmd(uint8_t IIC_Command);
// 写数据
void WriteDat(uint8_t IIC_Data);
// 初始化OLED
void OLED_Init(void);
// 设置起始点坐标
void OLED_SetPos(unsigned char x, unsigned char y);
// 开启电荷泵
void OLED_ON(void);
// 关闭电荷泵
void OLED_OFF(void);
// 刷新缓冲区数据到GDDRAM
void OLED_RefreshRAM(void);
// 清除数据缓冲区OLED_RAM buffer
void OLED_ClearRAM(void);
// 全屏填充
void OLED_FullyFill(uint8_t fill_Data);
// 清屏
void OLED_FullyClear(void);
// 这个是我写的显示一个小圈圈用的，你可以删除
void showO(unsigned char s);
// 获得坐标像素点数据
PixelStatus OLED_GetPixel(int16_t x, int16_t y);

/* 显示指定字符和图片时需要手动刷新缓冲区到GDDRAM
 * function list: OLED_ShowStr\OLED_ShowCN\OLED_Show_MixedCH\OLED_DrawBMP
 */
// 显示英文字符串
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char *ch, unsigned char TextSize);
// 显示整数用的
void OLED_ShowStr1(unsigned char x, unsigned char y, int *ch, unsigned char j, unsigned char TextSize);
// 显示中文字符串
void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N);
// 全屏垂直滚动播放
void OLED_VerticalShift(void);
// 全屏水平滚动播放
void OLED_HorizontalShift(uint8_t direction);
// 全屏同时垂直和水平滚动播放
void OLED_VerticalAndHorizontalShift(uint8_t direction);
// 屏幕内容取反显示
void OLED_DisplayMode(uint8_t mode);
// 屏幕亮度调节
void OLED_IntensityControl(uint8_t intensity);
//--------------------------------------------------------------
// Prototype      : void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);
// Calls          :
// Parameters     : x0,y0 -- 起始点坐标(x0:0~127, y0:0~7); x1,y1 -- 起点对角线(结束点)的坐标(x1:1~128,y1:1~8)
// Description    : 显示BMP位图
//--------------------------------------------------------------
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[]);
void OLED_PrintColume(uint8_t *text, uint8_t row);
char* Int2String(int num,char* str,int radix) ;

#endif
