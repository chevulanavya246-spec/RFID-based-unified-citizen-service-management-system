//spi.c
#include "types.h"
#include <LPC21xx.h>
#include "spi_defines.h"

void Init_SPI0(void)
{
  /* FIX: use |= not = so UART0 pins P0.0(TXD) and P0.1(RXD) are preserved */
  PINSEL0 |= SCK0|MISO0|MOSI0;
  S0SPCCR = 60;
  S0SPCR  = (1<<MSTR_BIT)|
             (1<<CPHA_BIT)|
             (1<<CPOL_BIT);
  IOSET0 = 1<<CS;
  IODIR0 |= 1<<CS;
}

u8 SPI0(u8 data)
{
   S0SPDR = data;
   while(((S0SPSR>>SPIF_BIT)&1)==0);
   return S0SPDR;
}
