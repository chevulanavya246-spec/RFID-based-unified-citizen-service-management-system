//spi_eeprom.c
#include <LPC21xx.h>
#include "types.h"
#include "spi_defines.h"
#include "spi_eeprom_defines.h"
#include "spi.h"
#include "delay.h"

void Cmd_25LC512(u8 cmd)
{
  IOCLR0=1<<CS;
  SPI0(cmd);                    //issue WREN/WRDI
  IOSET0=1<<CS;
}

void ByteWrite_25LC512(u16 wBufAddr,u8 dat)
{
  Cmd_25LC512(WREN);
  IOCLR0=1<<CS;
  SPI0(WRITE); 
  SPI0(wBufAddr>>8);
  SPI0(wBufAddr);
  SPI0(dat);
  IOSET0=1<<CS;
  delay_ms(10);
  Cmd_25LC512(WRDI);
}  

u8 ByteRead_25LC512(u16 rBufAddr)
{
  u8 dat;
  IOCLR0=1<<CS;
  SPI0(READ);   
  SPI0(rBufAddr>>8);
  SPI0(rBufAddr);   
  dat=SPI0(0x00);
  IOSET0=1<<CS;
  return dat;   
}  
