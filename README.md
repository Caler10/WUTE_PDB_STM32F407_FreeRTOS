# 针对WUTE车队电动方程式赛车开发的智能低压配电板 PDB

# 关于本项目

一款基于STM32F4+FreeRTOS开发的智能低压配电板 Power Delivery Board，具体功能如下：

1. 整车低压配电过流保护（取代保险丝）
2. 整车低压配电管理与诊断，可实时诊断各通道电流情况
3. 接收VCU的CAN报文实习对各设备配电控制和DCDC管理
4. 集成DCDC继电器、激活继电器
5. 带主动防护，包括过流保护、反接保护、TVS保护、过压保护等（仅限PDB_V1.1）