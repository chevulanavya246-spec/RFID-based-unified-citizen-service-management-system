//rtc.c

#include "types.h"
#include "lcd_defines.h"
#include "lcd.h"
#include <LPC21xx.h>
#include "rtc_defines.h"

//array to hold names of days of the week
s8 week[][4]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
//enum days{SUN,MON,TUE,WED,THU,FRI,SAT};

#define LPC2129 0
#define LPC2148 1
#define CPU LPC2148

void RTC_Init(void)
{
	CCR=RTC_RESET;

	#if(CPU==LPC2129)
	//SET PRESCALER 
	PREINT=PREINT_VAL;
	PREFRAC=PREFRAC_VAL;

	CCR=RTC_ENABLE;

	#elif(CPU==LPC2148)
	CCR=RTC_ENABLE | RTC_CLKSRC;

	#endif
	
}

void GetRTCTimeInfo(s32* hour,s32* minute,s32* second)
{
	*hour=HOUR;
	*minute=MIN;
	*second=SEC;
}

void DisplayRTCTimeInfo(u32 hour,u32 minute,u32 second)
{
	CmdLCD(GOTO_LINE1_POS0);
	CharLCD((hour/10)+48);
	CharLCD((hour%10)+48);
	CharLCD(':');
	CharLCD((minute/10)+48);
	CharLCD((minute%10)+48);
	CharLCD(':');
	CharLCD((second/10)+48);
	CharLCD((second%10)+48);
}

void GetRTCDateInfo(s32* date,s32* month,s32* year)
{
	*date=DOM;
	*month=MONTH;
	*year=YEAR;
}

void DisplayRTCDateInfo(u32 date,u32 month,u32 year)
{
	CmdLCD(GOTO_LINE2_POS0);
	CharLCD((date/10)+48);
	CharLCD((date%10)+48);
	CharLCD('/');
	CharLCD((month/10)+48);
	CharLCD((month%10)+48);
	CharLCD('/');
	U32LCD(year);	
}

void SetRTCTime(u32 hour,u32 minute,u32 second)
{
	HOUR=hour;
	MIN=minute;
	SEC=second;
}

void SetRTCDate(u32 date,u32 month,u32 year)
{
	DOM=date;
	MONTH=month;
	YEAR=year;
}

void GetRTCDay(s32* day)
{
	*day=DOW;
}

void DisplayRTCDay(u32 day)
{
	CmdLCD(GOTO_LINE1_POS0+11);
	StrLCD(week[day]);
}

void SetRTCDay(u32 day)
{
	DOW=day;
}

void SetRTCAlarm(u32 alarm_hour,u32 alarm_min)
{
	ALHOUR=alarm_hour;
	ALMIN=alarm_min;
}
void GetRTCAlarm(s32* alarm_hour,s32* alarm_min)
{
	*alarm_hour=ALHOUR;
	*alarm_min=ALMIN;
}

