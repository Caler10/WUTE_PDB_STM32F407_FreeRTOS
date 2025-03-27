
#if 1

#include "string.h"		   //为能使用strlen统计字符长度
#include "stm32f4xx_hal.h" //链接HAL库
#include "oledfont.h"	   //字库文件
#include "oled.h"		   //声明
#include "myusart.h"	   //声明

/* 控制宏 */ // 下面要用到
#define LEFT 0x27
#define RIGHT 0x26
#define UP 0X29
#define DOWM 0x2A
#define ON 0xA7
#define OFF 0xA6
#define OLED_ADDRESS 0x78
#define CHARACTERS_PER_ROW 16 // OLED每行能显示的字符数

/* IIC接口选择 */
#define IICx hi2c1
extern I2C_HandleTypeDef hi2c1; // HAL库使用，指定硬件IIC接口

// oled显示尺寸
uint16_t const displayWidth = 130; // 按道理应该是128，但是显示的时候有白边，所以改成130
uint16_t const displayHeight = 64;

/*  OLED显存
[0]0 1 2 3 ... 127
[1]0 1 2 3 ... 127
[2]0 1 2 3 ... 127
[3]0 1 2 3 ... 127
[4]0 1 2 3 ... 127
[5]0 1 2 3 ... 127
[6]0 1 2 3 ... 127
[7]0 1 2 3 ... 127 */

static uint8_t OLED_RAM[8][130]; // 定义GDDRAM缓存区（恕我直言没啥用）

/***************************************************
	I2C总线传出数据函数：
				addr  :    要写入的地址（OLED的地址一般为0x40;指令地址为0x00）
				data  :    要写入的数据
***************************************************/
void I2C_WriteByte(uint8_t addr, uint8_t data)
{
	uint8_t TxData[2] = {addr, data};
	HAL_I2C_Master_Transmit(&IICx, OLED_ADDRESS, (uint8_t *)TxData, 2, 10);
}

/**************************************************************
	 Prototype      : void WriteCmd(uint8_t IIC_Command)
	 Parameters     : IIC_Command
	 return					: none
	 Description    : 写命令（通过I2C_WriteByte中的HAL_I2C_Master_Transmit
					  向0x00写入命令）
***************************************************************/
void WriteCmd(uint8_t IIC_Command)
{
	I2C_WriteByte(0x00, IIC_Command);
}

/**************************************************************
	 Prototype      : void WriteDat(uint8_t IIC_Data)
	 Parameters     : IIC_Data
	 return					: none
	 Description    : 写数据（通过I2C_WriteByte中的HAL_I2C_Master_Transmit
					  向0x40写入数据）
***************************************************************/
void WriteDat(uint8_t IIC_Data)
{
	I2C_WriteByte(0x40, IIC_Data);
}

/**************************************************************
	 Prototype      : void OLED_Init(void)
	 Parameters     : none
	 return					: none
	 Description    : 初始化OLED模块（不难发现，其实都是一些向0x00写入的命令）
***************************************************************/
void OLED_Init(void)
{
//	HAL_Delay(500); // HAL延时函数

	WriteCmd(0xAE); // 开显示
	WriteCmd(0x20); // 设置内存寻址模式

	WriteCmd(0x10); // 00，水平寻址模式;01，垂直寻址模式;10，页面寻址模式(重置);11，无效
	WriteCmd(0xb0); // 为页面寻址模式设置页面开始地址，0-7
	WriteCmd(0x00); //---设置低列地址
	WriteCmd(0x10); //---设置高列地址

	WriteCmd(0xc8); // 设置COM输出扫描方向
	WriteCmd(0x40); //--设置起始行地址
	WriteCmd(0x81); //--set contrast control register
	WriteCmd(0xff); // 亮度调节 0x00~0xff
	WriteCmd(0xa1); //--设置段重新映射0到127
	WriteCmd(0xa6); //--设置正常显示
	WriteCmd(0xa8); //--设置复用比(1 ~ 64)
	WriteCmd(0x3F); //
	WriteCmd(0xa4); // 0xa4,输出遵循RAM内容;0xa5,Output忽略RAM内容
	WriteCmd(0xd3); //-设置显示抵消
	WriteCmd(0x00); //-not offset
	WriteCmd(0xd5); //--设置显示时钟分频/振荡器频率
	WriteCmd(0xf0); //--设置分率
	WriteCmd(0xd9); //--设置pre-charge时期
	WriteCmd(0x22); //
	WriteCmd(0xda); //--设置com大头针硬件配置
	WriteCmd(0x12);
	WriteCmd(0xdb); //--设置vcomh
	WriteCmd(0x20); // 0x20,0.77xVcc
	WriteCmd(0x8d); //--设置DC-DC	//设置电荷泵
	WriteCmd(0x14); // 开启电荷泵
	WriteCmd(0xaf); //--打开oled面板

	OLED_FullyClear(); // 清屏
}

void OLED_SetPos(unsigned char x, unsigned char y) // 设置起始点坐标
{
	WriteCmd(0xb0 + y); // y表示字符在哪一行，把0.96寸的屏幕分成0~7行，8个像素
						// 为一行，总高度64个像素
	WriteCmd(((x & 0xf0) >> 4) | 0x10); // x表示oled屏的行像素起点位置，表示每个矩阵的左上角坐标
	WriteCmd((x & 0x0f) | 0x01);
}

/**************************************************************
	 Prototype      : void OLED_ON(void)
	 Parameters     : none
	 return					: none
	 Description    : 将OLED从休眠中唤醒（初始化的时候已经完成了，你可以不考虑）
***************************************************************/
void OLED_ON(void)
{
	WriteCmd(0X8D); // 设置电荷泵
	WriteCmd(0X14); // 开启电荷泵
	WriteCmd(0XAF); // OLED唤醒
}

/**************************************************************
	 Prototype      : void OLED_OFF(void)
	 Parameters     : none
	 return					: none
	 Description    : 让OLED休眠 -- 休眠模式下,OLED功耗不到10uA
***************************************************************/
void OLED_OFF(void)
{
	WriteCmd(0X8D); // 设置电荷泵
	WriteCmd(0X10); // 关闭电荷泵
	WriteCmd(0XAE); // OLED休眠
}

/**************************************************************
	 Prototype      : void OLED_RefreshRAM(void)
	 Parameters     : none
	 return					: none
	 Description    : 全屏填充
***************************************************************/
void OLED_RefreshRAM(void)
{
	// 页寻址模式填充
	for (uint16_t m = 0; m < displayHeight / 8; m++)
	{
		WriteCmd(0xb0 + m); // 设置页地址b0~b7
		WriteCmd(0x00);		// 设置显示位置—列低地址00-0f
		WriteCmd(0x10);		// 设置显示位置—列高地址10-1f
		for (uint16_t n = 0; n < displayWidth; n++)
		{
			WriteDat(OLED_RAM[m][n]);
		}
	}
}

/**************************************************************
	 Prototype      : void OLED_ClearRAM(void)
	 Parameters     : none
	 return					: none
	 Description    : 清除数据缓冲区（前面说了，缓冲区，你用不到）
***************************************************************/
void OLED_ClearRAM(void)
{
	for (uint16_t m = 0; m < displayHeight / 8; m++)
	{
		for (uint16_t n = 0; n < displayWidth; n++)
		{
			OLED_RAM[m][n] = 0x00;
		}
	}
}

/**************************************************************
	 Prototype      : void OLED_Fill(uint8_t fill_Data)
	 Parameters     : fill_Data 填充的1字节数据
	 return					: none
	 Description    : 全屏填充 0x00~0xff
***************************************************************/
void OLED_FullyFill(unsigned char fill_Data)
{
	for (uint16_t m = 0; m < displayHeight / 8; m++)
	{
		for (uint16_t n = 0; n < displayWidth; n++)
		{
			OLED_RAM[m][n] = fill_Data;
		}
	}

	OLED_RefreshRAM();
}

/**************************************************************
	 Prototype      : void OLED_FullyClear(void)
	 Parameters     : none
	 return					: none
	 Description    : 全屏清除
***************************************************************/
void OLED_FullyClear(void)
{
	OLED_FullyFill(RESET_PIXEL);
}

/**************************************************************
	Prototype      :  void OLED_GetPixel(int16_t x, int16_t y)
	Parameters     : 	x,y -- 起始点坐标(x:0~127, y:0~63);
	return				 :  PixelStatus 像素点状态 	SET_PIXEL = 1, RESET_PIXEL = 0
	Description    : 	获得坐标像素点数据（对于0.96寸的屏幕来说，没啥用）
***************************************************************/
PixelStatus OLED_GetPixel(int16_t x, int16_t y)
{
	if (OLED_RAM[y / 8][x] >> (y % 8) & 0x01)
		return SET_PIXEL;

	return RESET_PIXEL;
}

/**************************************************************
	Prototype      : void OLED_ShowStr1(unsigned char x, unsigned char y, int *ch,
					 unsigned char s, unsigned char FontSize)
	Parameters     : 	x,y -- 起始点坐标(x:0~127, y:0~63);
						*ch -- 要显示的数字;
						s----- 数字的位数
						FontSize -- 字符大小(1:6*8 ; 2:8*16)
	return	       :    none
	Description    : 	显示codetab.h中的ASCII字符,有6*8和8*16可选择（我自己写的）
***************************************************************/
void OLED_ShowStr1(unsigned char x, unsigned char y, int *ch, unsigned char s, unsigned char FontSize)
{
	unsigned char c = 0, i = 0, j = 0, k = 0;
	switch (FontSize)
	{
	case 1:
	{
		while (s--)
		{
			c = (ch[j] + 16);
			if (x > 126)
			{
				x = 0;
				y++;
			}
			OLED_SetPos(x, y);
			for (i = 0; i < 6; i++)
				WriteDat(F6x8[c][i]);
			x += 6;
			j++;
		}
	}
	break;
	case 2:
	{
		while (s--)
		{
			c = (ch[j] + 16);
			if (x > 120)
			{
				x = 0;
				y++;
			}
			OLED_SetPos(x, y);
			for (i = 0; i < 8; i++)
				WriteDat(F8X16[c * 16 + i]);
			OLED_SetPos(x, y + 1);
			for (i = 0; i < 8; i++)
				WriteDat(F8X16[c * 16 + i + 8]);
			x += 8;
			j++;
		}
	}
	break;
	}
}

/**************************************************************
	 Prototype      : void OLED_ShowCN(int16_t x, int16_t y, uint8_t* n)
	 Parameters     : x,y -- 起始点坐标(x:0~127, y:0~7);
					  N[]:汉字在codetab.h中的索引（就是第几行）
	 return		    : none
	 Description    : 显示codetab.h中的汉字,16*16点阵
***************************************************************/

void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N)
{
	unsigned char wm = 0;
	unsigned int adder = 32 * N;
	OLED_SetPos(x, y);
	for (wm = 0; wm < 16; wm++)
	{
		WriteDat(F16x16[adder]);
		adder += 1;
	}
	OLED_SetPos(x, y + 1);
	for (wm = 0; wm < 16; wm++)
	{
		WriteDat(F16x16[adder]);
		adder += 1;
	}
}
/**************************************************************
	 Prototype      :	void OLED_FullyToggle(void)
	 Parameters     : none
	 return				  : none
	 Description    : 缓冲区数据取反后刷新到GDDRAM（懂我意思吧）
***************************************************************/
void OLED_FullyToggle(void)
{
	for (uint16_t m = 0; m < displayHeight / 8; m++)
	{
		for (uint16_t n = 0; n < displayWidth; n++)
		{
			OLED_RAM[m][n] = ~OLED_RAM[m][n];
		}
	}
	OLED_RefreshRAM();
}

/****************************************************************
	全屏垂直偏移,0->63方向
	方向垂直向上,范围0-63
	方向垂直向下,范围63-0
****************************************************************/
void OLED_VerticalShift(void)
{
	for (uint8_t i = 0; i < displayHeight; i++)
	{
		WriteCmd(0xd3); // 设置显示偏移，0->63方向
		WriteCmd(i);	// 偏移量
		HAL_Delay(40);	// 延时时间
	}
}

/****************************************************************
	屏幕内容水平全屏滚动播放
	左		LEFT	0x27
	右		RIGHT	0x26
****************************************************************/
void OLED_HorizontalShift(uint8_t direction)

{
	WriteCmd(direction); // 设置滚动方向
	WriteCmd(0x00);		 // 虚拟字节设置，默认为0x00
	WriteCmd(0x00);		 // 设置开始页地址
	WriteCmd(0x05);		 // 设置每个滚动步骤之间的时间间隔的帧频
	WriteCmd(0x07);		 // 设置结束页地址
	WriteCmd(0x00);		 // 虚拟字节设置，默认为0x00
	WriteCmd(0xff);		 // 虚拟字节设置，默认为0xff
	WriteCmd(0x2f);		 // 开启滚动-0x2f，禁用滚动-0x2e，禁用需要重写数据
}

/****************************************************************
	屏幕内容垂直水平全屏滚动播放
	上		UP		0x29
	下		DOWN	0x2A
****************************************************************/
void OLED_VerticalAndHorizontalShift(uint8_t direction)
{
	WriteCmd(direction); // 设置滚动方向
	WriteCmd(0x00);		 // 虚拟字节设置，默认为0x00
	WriteCmd(0x00);		 // 设置开始页地址
	WriteCmd(0x05);		 // 设置每个滚动步骤之间的时间间隔的帧频
	WriteCmd(0x07);		 // 设置结束页地址
	WriteCmd(0x01);		 // 垂直滚动偏移量

	WriteCmd(0x2f); // 开启滚动-0x2f，禁用滚动-0x2e，禁用需要重写数据
}

/****************************************************************
	屏幕内容取反显示
	开	ON	0xA7
	关	OFF	0xA6	默认此模式，设置像素点亮
****************************************************************/
void OLED_DisplayMode(uint8_t mode)
{
	WriteCmd(mode);
}

/****************************************************************
	屏幕亮度调节
	intensity	0-255
	默认为0x7f
****************************************************************/
void OLED_IntensityControl(uint8_t intensity)
{
	WriteCmd(0x81);
	WriteCmd(intensity);
}

// X0是图像的起始位置，y0是图像的起始行，X1是图像的宽度，Y1是图像的显示高度1~8；
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char *BMP)
{
	unsigned int j = 0;
	unsigned char x, y;

	if (y1 % 8 == 0)
		y = y1 / 8;
	else
		y = y1 / 8 + 1;
	for (y = y0; y < y1; y++)
	{
		OLED_SetPos(x0, y);
		for (x = x0; x < x1; x++)
		{
			WriteDat(BMP[j++]);
		}
	}
}

/************************************************************
	Prototype      : void OLED_ShowStr1(unsigned char x, unsigned char y, int *ch,
					 unsigned char s, unsigned char FontSize)
	Parameters     : 	x,y -- 起始点坐标(x:0~127, y:0~7);
						*ch -- 要显示的ASCII字符,可以直接传递字符串;
						FontSize -- 字符大小(1:6*8 ; 2:8*16)
	return	       :    none
	Description    : 	显示codetab.h中的ASCII字符,有6*8和8*16可选择
 ***********************************************************/
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char *ch, unsigned char FontSize)
{
	unsigned char c = 0, i = 0, j = 0;
	switch (FontSize)
	{
	case 1:
	{
		while (ch[j] != '\0')
		{
			c = ch[j] - 32;
			if (x > 126)
			{
				x = 0;
				y++;
			}
			OLED_SetPos(x, y);
			for (i = 0; i < 6; i++)
				WriteDat(F6x8[c][i]);
			x += 6;
			j++;
		}
	}
	break;
	case 2:
	{
		while (ch[j] != '\0')
		{
			c = ch[j] - 32;
			if (x > 120)
			{
				x = 0;
				y++;
			}
			OLED_SetPos(x, y);
			for (i = 0; i < 8; i++)
				WriteDat(F8X16[c * 16 + i]);
			OLED_SetPos(x, y + 1);
			for (i = 0; i < 8; i++)
				WriteDat(F8X16[c * 16 + i + 8]);
			x += 8;
			j++;
		}
	}
	break;
	}
}

// 显示一个小圈圈用的
void showO(unsigned char s)
{
	extern const unsigned char O[];
	unsigned char i, k;
	for (i = 0; i < 4; i++)
	{
		switch (s)
		{
		case 1:
			k = 98 + s;
			break;
		case 2:
			k = 106 + s;
			break;
		case 3:
			k = 114 + s;
			break;
		}
		OLED_DrawBMP(k, 1, k + 4, 2, (unsigned char *)O);
	}
}

void OLED_PrintColume(uint8_t *text, uint8_t row)
{
	//	 uint32_t num_rows = (strlen(text) + CHARACTERS_PER_ROW - 1) / CHARACTERS_PER_ROW; // 向上取整

	//	     if (row < 1 || row > num_rows) {
	//	         myprintf("oled over row");
	//	 		return;
	//	     }//错误处理

	//	    uint32_t start_index = (row - 1) * CHARACTERS_PER_ROW;
	//	    uint32_t end_index = start_index + CHARACTERS_PER_ROW;
	//	    if (end_index > strlen(text)) {
	//	        end_index = strlen(text);
	//	    }//起始与结束检索

	//	    uint32_t display_text[CHARACTERS_PER_ROW + 1];//发送的字符串
	//	    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS, display_text, sizeof(display_text), HAL_MAX_DELAY);
	//	    display_text[end_index - start_index] = '\0';
}

//？进制数转字符串函数
char* Int2String(int num,char* str,int radix) 
{
    char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";//索引表
    unsigned unum;//存放要转换的整数的绝对值,转换的整数可能是负数
    int i=0,j,k;//i用来指示设置字符串相应位，转换之后i其实就是字符串的长度；转换后顺序是逆序的，有正负的情况，k用来指示调整顺序的开始位置;j用来指示调整顺序时的交换。
 
    //获取要转换的整数的绝对值
    if(radix==10&&num<0)//要转换成十进制数并且是负数
    {
        unum=(unsigned)-num;//将num的绝对值赋给unum
        str[i++]='-';//在字符串最前面设置为'-'号，并且索引加1
    }
    else unum=(unsigned)num;//若是num为正，直接赋值给unum
 
    //转换部分，注意转换后是逆序的
    do
    {
        str[i++]=index[unum%(unsigned)radix];//取unum的最后一位，并设置为str对应位，指示索引加1
        unum/=radix;//unum去掉最后一位
 
    }while(unum);//直至unum为0退出循环
 
    str[i]='\0';//在字符串最后添加'\0'字符，c语言字符串以'\0'结束。
 
    //将顺序调整过来
    if(str[0]=='-') k=1;//如果是负数，符号不用调整，从符号后面开始调整
    else k=0;//不是负数，全部都要调整
 
    char temp;//临时变量，交换两个值时用到
    for(j=k;j<=(i-1)/2;j++)//头尾一一对称交换，i其实就是字符串的长度，索引最大值比长度少1
    {
        temp=str[j];//头部赋值给临时变量
        str[j]=str[i-1+k-j];//尾部赋值给头部
        str[i-1+k-j]=temp;//将临时变量的值(其实就是之前的头部值)赋给尾部
    }
 
    return str;//返回转换后的字符串
}


#endif
