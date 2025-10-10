# 针对WUTE车队电动方程式赛车开发的智能低压配电板 PDB

# 关于本项目

一款基于STM32F4+FreeRTOS开发的智能低压配电板(PDB,Power Delivery Board)，具体功能如下：

1. 支持多路低压配电过流保护与诊断（基于智能高边驱动芯片，取代保险丝）
2. 支持对接入设备实现配电控制与管理，实现智能化配电功能（基于智能高边驱动芯片，取代继电器）
3. 负责赛车车身风扇、水泵、尾灯、蜂鸣器、DRS等设备的控制
4. 接收并解析来自VCU的配电CAN报文指令和来自仪表的DRS启停CAN报文，同时发送各路配电诊断数据、各车身设备启停状态至CAN总线
5. 集成DCDC继电器、激活继电器
6. 支持过流保护、反接保护、TVS保护、过压保护等多项安全功能

# 设计说明

## 硬件说明

因整车低压配电需要的载流较大，担心智能低压配电板由于设计缺陷导致需要重新设计和打板，为降低开发成本，驱动电路和控制电路采用分板设计（核心板+底板）

智能低压配电板架构图如下：

![PDB_V1.1_MultiBoard_Architecture.png](Hardware/PDB_V1.1/PDB_V1.1_MultiBoard_Architecture.png)

### PDB叠板设计

PDB的核心板和底板之间采用板对板连接器连接

PDB_V1.0_MultiBoard：

![PDB_V1.0_MultiBoard_3D.png](Hardware/PDB_V1.0/PDB_V1.0_MultiBoard_3D.png)

PDB_V1.1_MultiBoard：

![PDB_V1.1_MultiBoard_3D.png](Hardware/PDB_V1.1/PDB_V1.1_MultiBoard_3D.png)

PDB_V1.0集成了OLED实时诊断数据显示，但是观察灵活性不够，因此PDB_V1.1取消了OLED，集成了4G数传功能，方便远程监测赛车配电情况，另外PDB_V1.1在底板增加了烧录、CAN调试接口，并优化了配电载流走线、修复了一些器件PCB封装问题

### PDB核心板设计

PDB_V1.0核心板：

V1.0核心板支持外接OLED，实现诊断数据实时显示

![PDB_V1.0_Core_2D.png](Hardware/PDB_V1.0/PDB_V1.0_Core_2D.png)

![PDB_V1.0_Core_3D.png](Hardware/PDB_V1.0/PDB_V1.0_Core_3D.png)

PDB_V1.1核心板：

V1.1核心板为兼容4G模块增加了3.3V至1.8V的电平转换模块，同时取消了OLED接口，另外相比V1.0版本增加了过流保护和反接保护电路

![PDB_V1.1_Core_2D.png](Hardware/PDB_V1.1/PDB_V1.1_Core_2D.png)

![PDB_V1.1_Core_3D.png](Hardware/PDB_V1.1/PDB_V1.1_Core_3D.png)

### PDB底板设计

PDB_V1.0底板：

![PDB_V1.0_Mother_3D.png](Hardware/PDB_V1.0/PDB_V1.0_Mother_3D.png)

![PDB_V1.0_Mother_3Dback.png](Hardware/PDB_V1.0/PDB_V1.0_Mother_3Dback.png)

PDB_V1.1底板：

![PDB_V1.1_Mother_3D.png](Hardware/PDB_V1.1/PDB_V1.1_Mother_3D.png)

![PDB_V1.1_Mother_3Dback.png](Hardware/PDB_V1.1/PDB_V1.1_Mother_3Dback.png)

## 软件说明

开发平台：STM32F407VET6

开发环境：STM32cubeMX+Keil

软件架构：基于CORE层+BSP层+FreeRTOS+APP层的架构开发

软件功能：

1. 智能高边驱动芯片HSD集群GPIO/PWM驱动
   
2. HSD诊断脚ADC+DMA中断采集与诊断分析
   
3. OLED IIC驱动与诊断可视化
   
4. CAN报文中断接收解析与诊断信息定时发送
   
5. 基于事件组/信号量/队列的任务间通信等

![PDB_firmware.png](Firmware/PDB_firmware.png)

# 实物图

PDB实物图：

![PDB_V1.1_Picture.png](Hardware/PDB_V1.1_Picture.png)

PDB和两块低压电池集成封装，布置在赛车尾部

![PDB_V1.1_Picture2.png](Hardware/PDB_V1.1_Picture2.png)

# 项目成果

该项目顺利完成开发并搭载在2024赛季E10赛车上，通过了累计50+小时的耐久测试，有效保障赛车低压系统配电安全同时实现了整车轻量化（采用HSD取代保险丝和继电器，实现配电盒整体减重约30%），并支撑E10赛车在2024年FSAE中国大学生电动方程式汽车大赛顺利完赛并取得全国季军。
