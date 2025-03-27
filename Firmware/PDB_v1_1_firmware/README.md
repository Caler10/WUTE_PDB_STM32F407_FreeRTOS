MX生成FreeRTOS版本（后面还有一个自己移植FreeRTOS版本）


由于电池VBAT始终向PDB供电，所以单片机不会关机，考虑读取PA0(上GLVMS为高，下GLVMS为低电平)

软件流程

当处于休眠时，监测到PA0上升沿唤醒

初始化阶段：
RST、上电后如果PA0读取非高电平（后可加其他条件，如CAN开始无应答、监听时），1分钟后进入休眠
如果PA0为高电平
    OLED开机界面启动
    CAN中断读取启动
    HSD所有输出开启
    OLED退出开机界面，进入指示状态

进入任务调度
    HSD1、HSD2诊断与分析（定时执行）[两队列：传出HSD诊断数据、报警信号；两事件：一出现报警信息，数据传输事件]（报过警的不丢,也显示，但说明过去式）
    电压监测与分析（定时执行）[两队列：电压监测数据、报警信号；两事件：一出现报警信息，数据传输事件]
    OLED监视（HSD电流检测数据来临事件启动）[收事件]
    CAN发送（HSD电流检测数据来临事件启动）[收事件]
    CAN接收（消息来临标志位启动）[收]
    HSD通道启停控制（CAN接收控制信号事件启动）[]
    DRS控制（CAN接收控制信号事件启动）[]
    准备与取消准备休眠（PA0 GPIO中断读取，为低电平后OLED指示将1分钟后进入休眠，当PA0回高电平时退出准备）[]
    休眠（由进入休眠事件启动，为低电平后倒计时1分钟后关闭LED、OLED，将PA0复用为唤醒引脚，配置寄存器进入休眠）[]

疑问：
有什么方式实现报警发过了不再发？
现在CAN接收标准帧和拓展帧都能收？

为什么OLED字符、图片数组声明、定义前要加const使其常数化（但是加了传入画图片函数里时会警告，所以去掉了）
小心多个任务读取同一个事件位时，一个读完了把厕所炸了其他的就上不了了


MX生成FreeRTOS的使用:
大部分FreeRTOS功能函数还要include对应文件后才能使用

HSD1handledEvent：存放HSD1组的完成诊断的相对输出口数据


CAN发尾灯信号，PDB控制
// 整合电压电流采集的两数据（数组）二维数组转一维数组，参考https://blog.csdn.net/m0_54158068/article/details/124275490

//有的地方set事件改为清理事件
uxBits = xEventGroupClearBits(
                                xEventGroup,  /* The event group being updated. */
                                BIT_0 | BIT_4 );/* The bits being cleared. */

  if( ( uxBits & ( BIT_0 | BIT_4 ) ) == ( BIT_0 | BIT_4 ) )
  {
      /* Both bit 0 and bit 4 were set before xEventGroupClearBits()
      was called.  Both will now be clear (not set). */
  }
  else if( ( uxBits & BIT_0 ) != 0 )
  {
      /* Bit 0 was set before xEventGroupClearBits() was called.  It will
      now be clear. */
  }
  else if( ( uxBits & BIT_4 ) != 0 )
  {
      /* Bit 4 was set before xEventGroupClearBits() was called.  It will
      now be clear. */
  }
  else
  {
      /* Neither bit 0 nor bit 4 were set in the first place. */
  }

20240524：
比20240413版本
解决了风扇无法控制的问题（CAN中断接收到后不能将数据完整地压入队列供CAN_Reseive函数接收处理）
取消诊断、诊断数据打包发送、错误发送功能（控制功耗）
删除CPU占用监测任务

20240704：
实现DRS的驱动
可以正常使用版本

20240728：
更新PDB指令报文的接收
实现尾灯、风扇PWM控制
PDB设备状态数据上CAN（给仪表）
实现惯导转发上CAN
实现4G模块驱动
