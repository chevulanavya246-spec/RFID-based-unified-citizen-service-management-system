//lcd.h
#include "types.h"
void WriteLCD(u8 byte);
void CmdLCD(u8 cmd);
void InitLCD(void);
void CharLCD(u8 asciiVal);
void StrLCD(s8 *s);
void U32LCD(u32);
void S32LCD(s32);
void F32LCD(f32 fn,u8 nDP);
void BuildCGRAM(u8 *p,u8 nBytes);
