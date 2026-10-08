// uart.h

#ifndef __UART_H__
#define __UART_H__

#include "types.h"

void UART0_Init(void);
void UART0_Tx(u8 data);
u8 UART0_Rx(void);
void UART0_SendString(const char *str);
void UART0_SendInteger(u32 val);
void UART0_ISR(void) __irq;

#endif
