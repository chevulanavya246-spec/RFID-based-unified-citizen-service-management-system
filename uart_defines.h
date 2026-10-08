// uart_defines.h

#ifndef __UART_DEFINES_H__
#define __UART_DEFINES_H__

/* PIN FUNCTION CONFIGURATION */
#define TXD0_PIN_FUNC      0x00000001
#define RXD0_PIN_FUNC      0x00000004

/* UART SETTINGS */
#define BAUD               9600

/* CLOCK SETTINGS */
#define FOSC               12000000
#define CCLK               (FOSC * 5)
#define PCLK               (CCLK / 4)

/* BAUD RATE DIVISOR */
#define DIVISOR            (PCLK / (16 * BAUD))

/* U0LCR REGISTER BITS */
#define WORD_LEN_SEL_BITS  0
#define DLAB_BIT           7

/* 8-BIT MODE */
#define _8BIT              3

/* U0LSR REGISTER BITS */
#define TEMT_BIT           6
#define DR_BIT             0

/* U0IER/U0IIR interrupt bits used by UART0 RX interrupt. */
#define RBR_INT_BIT        0
#define UART0_VIC_CHANNEL  6
#define UART_IIR_NO_INT    1
#define UART_IIR_RDA       2
#define UART_IIR_CTI       6

#endif
