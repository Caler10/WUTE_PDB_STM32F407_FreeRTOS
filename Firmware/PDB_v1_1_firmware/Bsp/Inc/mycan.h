#ifndef __MYCAN_H
#define __MYCAN_H

#include "main.h"

void CAN_Init(void);
void CAN1_Filter_Config(void);
uint8_t CAN_Transmit(const void *Buf , uint32_t Len ,uint32_t CAN_ID );

#endif //__MYCAN_H
