//rtc.h
#include "types.h"

void RTC_Init(void);
void GetRTCTimeInfo(s32*,s32*,s32*);
void DisplayRTCTimeInfo(u32,u32,u32);
void GetRTCDateInfo(s32*,s32*,s32*);
void DisplayRTCDateInfo(u32,u32,u32);
void SetRTCTime(u32,u32,u32);
void SetRTCDate(u32,u32,u32);
void GetRTCDay(s32*);
void DisplayRTCDay(u32);
void SetRTCDay(u32);

