/*=============================================================
 * kpm1.h
 *=============================================================*/
#ifndef KPM_H
#define KPM_H

#include "types.h"
#include "kpm1_defines.h"

void InitKPM(void);
u8   colscan(void);
u8   rowcheck(void);
u8   colcheck(void);
u8   keyscan(void);     /* returns ASCII: '0'-'9','*','#','C' */
u32  ReadNum(void);     /* digits shown as numbers on LCD     */
void ReadPass(u8 *buf); /* digits shown as * on LCD           */

#endif
