//spi_eeprom.h
#include "types.h"
void Cmd_25LC512(u8 cmd);
void ByteWrite_25LC512(u16 wBufAddr,u8 dat);
u8   ByteRead_25LC512(u16 rBufAddr);
