#if 1

#include "can.h"
#include "mycan.h"
#include "myusart.h" //现在是耦合在里面地使用

#define CAN1_FILTER_MODE_MASK_ENABLE 1 //CAN1过滤器模式选择：=1：屏蔽位模式  =0：屏蔽列表模式
#define CAN1_ID 0x1111FFFF             //PDB的CAN ID（未采用）
#define CAN1_FILTER_BANK 0             //主CAN过滤器组编号

CAN_RxHeaderTypeDef RxHeader;
CAN_TxHeaderTypeDef TxHeader;

uint8_t CAN1_TX_BUF[8]; ///< CAN1数据发送缓存

/// CAN初始化
void CAN_Init(void)
{
//    MX_CAN1_Init();                                                    // 初始化CNA1,已操作
    CAN1_Filter_Config();                                              // 初始化CNA1过滤器
    HAL_CAN_Start(&hcan1);                                             // 启动CAN1
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING); // 激活CAN1 FIFO0
}

// 设置CAN1的过滤器（主CAN），现在为掩码模式，掩码为0，全接收
void CAN1_Filter_Config(void)
{
    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = CAN1_FILTER_BANK; // 设置过滤器组编号
#if CAN1_FILTER_MODE_MASK_ENABLE
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; // 屏蔽位模式
#else
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST; // 列表模式
#endif
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; // 32位宽
    sFilterConfig.FilterIdHigh = 0;                    // 标识符寄存器一ID高十六位，放入扩展帧位
    sFilterConfig.FilterIdLow = 0;                     // 标识符寄存器一ID低十六位，放入扩展帧位
    sFilterConfig.FilterMaskIdHigh = 0;                // 标识符寄存器二ID高十六位，放入扩展帧位
    sFilterConfig.FilterMaskIdLow = 0;             // 标识符寄存器二ID低十六位，放入扩展帧位
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; // 过滤器组关联到FIFO0
    sFilterConfig.FilterActivation = ENABLE;           // 激活过滤器

    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    // 这里可加 myprintf 打印必要信息
}

/**
 * CAN数据传输
 * @param  buf    待发送的数据
 * @param  len    数据长度（要指示清楚！！！然后下面帮你自动分帧发送）
 * @param  number CAN编号，=0：CAN1，=1：CAN2
 * @return        0：成功  other：失败
 */

uint8_t CAN_Transmit(const void *Buf , uint32_t Len ,uint32_t CAN_ID )
{
    uint32_t txmailbox = 0;
    uint32_t offset = 0;

	//    TxHeader.StdId = 0;          // 标准帧ID,最大11位，也就是0x7FF（使用的标准帧，不配置）
    TxHeader.ExtId = CAN_ID;    // 扩展帧ID,最大29位，也就是0x1FFFFFFF
    TxHeader.IDE = CAN_ID_EXT;   // ID类型：扩展帧
    TxHeader.RTR = CAN_RTR_DATA; // 帧类型：数据帧
                                 //    TxHeader.DLC = 8;															//数据段长度
    TxHeader.TransmitGlobalTime = DISABLE;

    while (Len != 0)
    {
        TxHeader.DLC = Len > 8 ? 8 : Len; // 数据长度
                                          //        if (HAL_CAN_AddTxMessage(number == 0 ? &hcan1 : &hcan2, &hdr, ((uint8_t *)buf) + offset, &txmailbox) != HAL_OK)
        if (HAL_CAN_AddTxMessage(&hcan1, &TxHeader, ((uint8_t *)Buf) + offset, &txmailbox) != HAL_OK)
            return 1;
        offset += TxHeader.DLC;
        Len -= TxHeader.DLC;
    }
    return 0;
}

#endif
