
#ifndef __CAN_APP_H__
#define __CAN_APP_H__

#include "main.h"

// 实现摆脱看值的“宏定义”
//typedef enum
//{
//  aOK = 0x00,
//  aERROR = 0x01,
//  aBUSY = 0x02,
//  aTIMEOUT = 0x03
//} MonitorErrorStatus;



void CAN_Data_Send_App(void);
void CAN_Error_Send_App(void);
void CAN_Recv_App(void);

// inline void CAN_RecvHandler(void);

#endif
