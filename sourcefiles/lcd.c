//lcd.c
#include <LPC21xx.h>
#include "lcd_defines.h"
#include "defines.h"
#include "types.h"
#include "delay.h"

void WriteLCD(u8 byte)
{
	//select write option
	IOCLR0=1<<LCD_RW;
	//PLACE ANY BYTE ON DATA PINS D0 TO D7
	WRITEBYTE(IOPIN0,LCD_DATA,byte);
	//PROVIDE HIGH TO LOW PULSE FOR LATCHING
	IOSET0=1<<LCD_EN;
	delay_us(1);
	IOCLR0=1<<LCD_EN;
	delay_ms(2);
}

void CmdLCD(u8 cmd)
{
	//SELECT COMMAND REGISTER 
	IOCLR0=1<<LCD_RS;
	//WRITE ANY CMD TO LED
	WriteLCD(cmd);
}

void InitLCD(void)
{
	//cfg led connection gpio o/p pins
	WRITEBYTE(IODIR0,LCD_DATA,0xFF);	 //P08 TO P0.15
	SETBIT(IODIR0,LCD_RS);				 //P0.16
	SETBIT(IODIR0,LCD_RW);				 //P0.17
	SETBIT(IODIR0,LCD_EN);				 //P0.18
	//POWER ON DELAY
	delay_ms(15);
	CmdLCD(0X30);
	delay_ms(4);
	delay_us(100);
	CmdLCD(0X30);
	delay_us(100);
	CmdLCD(0x30);
	CmdLCD(MODE_8BIT_2LINE);
	CmdLCD(DSP_ON_CUR_BLINK);
	CmdLCD(CLEAR_LCD);
	CmdLCD(SHIFT_CUR_RIGHT);
}

void CharLCD(u8 asciiVal)
{
	//select data register
		IOSET0=1<<LCD_RS;
		//write to DDRAM via data register
		WriteLCD(asciiVal);
}

void StrLCD(s8 *s)
{
	while(*s)
		CharLCD(*s++);
}

void U32LCD(u32 n)
{
	s32 i=0;
	u8 a[10];
	if(n==0)
		CharLCD('0');
	else
	{
		while(n>0)
		{
			a[i++]=(n%10)+48;
			n/=10;
		}
		for(--i;i>=0;i--)
		{
			CharLCD(a[i]);
		}
	}
}

void S32LCD(s32 n)
{
	if(n<0)
	{
		CharLCD('-');
		n=-n;
	}
	U32LCD(n);
}

void F32LCD(f32 fn,u8 nDP)
{
	u32 n,i;
	if(fn<0.0)
	{
		CharLCD('-');
		fn=-fn;
	}
	n=fn;
	U32LCD(n);
	CharLCD('.');
	for(i=0;i<nDP;i++)
	{
		fn=(fn-n)*10;
		n=fn;
		CharLCD(n+48);
	}
}

void BuildCGRAM(u8 *p,u8 nbytes)
{
	u32 i;
	CmdLCD(GOTO_CGRAM_START);
	IOCLR0=1<<LCD_RW;
	IOSET0=1<<LCD_RS;
	for(i=0;i<nbytes;i++)
	{
		//write to CGRAM via dta register
		WriteLCD(p[i]);
	}
	//return back DDRAM
	CmdLCD(GOTO_LINE1_POS0);
}
