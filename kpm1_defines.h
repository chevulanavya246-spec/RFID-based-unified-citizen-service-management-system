/*=============================================================
 * kpm_defines.h
 * 4x4 Matrix Keypad — LPC2148
 *
 * ROW0=P1.16  ROW1=P1.17  ROW2=P1.18  ROW3=P1.19  (OUTPUT)
 * COL0=P1.20  COL1=P1.21  COL2=P1.22  COL3=P1.23  (INPUT)
 *=============================================================*/

#ifndef KPM_DEFINES_H
#define KPM_DEFINES_H

#define ROW0    16      /* P1.16                             */
#define COL0    20      /* P1.20                             */

#define MAX_LEN  5      /* max digits for number/password    */
#define PASS_LEN 4
#endif
