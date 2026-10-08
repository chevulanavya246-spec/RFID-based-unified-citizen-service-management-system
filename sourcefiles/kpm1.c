/*=============================================================
 * kpm.c
 * 4x4 Matrix Keypad — LPC2148
 *
 * LUT stores ASCII directly — no index mapping needed.
 *
 * Keypad layout:
 *   [ 1 ][ 2 ][ 3 ][ A ]   C = CLEAR
 *   [ 4 ][ 5 ][ 6 ][ B ]   * = BACKSPACE
 *   [ 7 ][ 8 ][ 9 ][ C ]   # = ENTER
 *   [ * ][ 0 ][ # ][ D ]   blank = ignored
 *
 * LCD functions used from your lcd.c:
 *   CmdLCD(cmd)     CharLCD(c)    StrLCD(s)    U32LCD(n)
 *
 * LCD defines used from your lcd_defines.h:
 *   CLEAR_LCD   GOTO_LINE1_POS0   GOTO_LINE2_POS0
 *   and positions: GOTO_LINE2_POS0+n for column offset
 *=============================================================*/

#include <LPC21xx.h>
#include "types.h"
#include "defines.h"
#include "kpm1_defines.h"
#include "kpm1.h"
#include "lcd.h"
#include "lcd_defines.h"
#include "delay.h"

/*-------------------------------------------------------------
 * LUT — stores actual ASCII characters
 *
 * Physical layout:          LUT:
 *  [1][2][3][A]    →    {'1','2','3','A'}
 *  [4][5][6][B]    →    {'4','5','6','B'}
 *  [7][8][9][C]    →    {'7','8','9','C'}
 *  [*][0][#][D]    →    {'*','0','#','D'}
 *
 * keyscan() returns ASCII directly:
 *   '0'-'9' = digit keys
 *   '*'     = backspace
 *   '#'     = enter
 *   'C'     = clear
 *   ' '     = ignored
 *-------------------------------------------------------------*/
u8 kpmLUT[4][4] = {
    {'1', '2', '3', 'A'},   /* row0: 1  2  3  CLEAR     */
    {'4', '5', '6', 'B'},   /* row1: 4  5  6  BACKSPACE */
    {'7', '8', '9', 'C'},   /* row2: 7  8  9  ENTER     */
    {'*', '0', '#', 'D'}    /* row3: -  0  -  -         */
};

/*=============================================================
 * InitKPM()
 *=============================================================*/
void InitKPM(void)
{
    WRITENIBBLE(IODIR1, ROW0, 15);     /* ROW = output        */
    WRITENIBBLE(IOPIN1, ROW0, 0);      /* all rows grounded   */
}

/*=============================================================
 * colscan()
 * Returns 0 = key pressed, 1 = no key
 *=============================================================*/
u8 colscan(void)
{
    u8 t;
    t = (READNIBBLE(IOPIN1, COL0) < 15) ? 0 : 1;
    return t;
}

/*=============================================================
 * rowcheck()
 * Returns row index 0-3 of the pressed key
 *=============================================================*/
u8 rowcheck(void)
{
    u8 r;
    for(r = 0; r < 4; r++)
    {
        WRITENIBBLE(IOPIN1, ROW0, ~(1 << r));
        if(!colscan())
            break;
    }
    WRITENIBBLE(IOPIN1, ROW0, 0);
    return r;
}

/*=============================================================
 * colcheck()
 * Returns col index 0-3 of the pressed key
 *=============================================================*/
u8 colcheck(void)
{
    u8 c;
    for(c = 0; c < 4; c++)
    {
        if(READBIT(IOPIN1, COL0 + c) == 0)
            break;
    }
    return c;
}

/*=============================================================
 * keyscan()
 * Waits for key press, returns ASCII character directly.
 * No index conversion needed — LUT has ASCII values.
 *=============================================================*/
u8 keyscan(void)
{
    u8 r, c;

    /* wait for any press */
    while(colscan());

    r = rowcheck();
    c = colcheck();

    return kpmLUT[r][c];    /* direct ASCII from LUT */
}

/*=============================================================
 * ReadNum()
 *
 * Number input — digits shown as actual numbers on LCD.
 *
 * '0'-'9'  → show digit on LCD, add to sum
 * '*'      → BACKSPACE: remove last digit (sum/10), erase LCD
 * 'C'      → CLEAR: reset sum, clear LCD line2
 * '#'      → ENTER: return sum
 * ' '      → ignored
 *
 * LCD:
 *   Line1:  Enter Number:
 *   Line2:  123_
 *=============================================================*/
u32 ReadNum(void)
{
    u8  keyV;
    u32 sum       = 0;
    u8  dig_count = 0;

    //CmdLCD(CLEAR_LCD);
    //CmdLCD(GOTO_LINE1_POS0);
    //StrLCD((s8*)"Enter Number:");
    CmdLCD(GOTO_LINE2_POS0);
	delay_ms(20);

    while(1)
    {
        keyV = keyscan();
		delay_ms(100);

        /*--- Digit key -------------------------------------*/
        if(keyV >= '0' && keyV <= '9')
        {
            if(dig_count < MAX_LEN)
            {
                sum = (sum * 10) + (keyV - '0');
                dig_count++;

                CmdLCD(GOTO_LINE2_POS0);
                U32LCD(sum);
            }
            while(colscan() == 0);  /* wait release */
        }

        /*--- Backspace '*' ---------------------------------*/
        else if(keyV == '*')
        {
            if(dig_count > 0)
            {
                sum = sum / 10;
                dig_count--;

                /* clear line2 then redraw */
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD((s8*)"     ");
                CmdLCD(GOTO_LINE2_POS0);
                if(dig_count > 0)
                    U32LCD(sum);
            }
            while(colscan() == 0);
        }

        /*--- Clear 'C' -------------------------------------*/
        else if(keyV == 'C')
        {
            sum       = 0;
            dig_count = 0;

            CmdLCD(GOTO_LINE2_POS0);
            StrLCD((s8*)"     ");
            CmdLCD(GOTO_LINE2_POS0);

            while(colscan() == 0);
        }

        /*--- Enter '#' -------------------------------------*/
        else if(keyV == '#')
        {
            while(colscan() == 0);
            break;
        }

        /* ' ' ignored */
        else 
		{ 
			while(colscan() == 0); 
		}
    }

    return sum;
}

/*=============================================================
 * ReadPass(buf)
 *
 * Password input — digits shown as '*' on LCD.
 *
 * '0'-'9'  → store in buf[], show '*' on LCD
 * '*'      → BACKSPACE: remove last '*' from LCD, clear buf[i]
 * 'C'      → CLEAR: wipe all '*', reset buf
 * '#'      → ENTER: null-terminate buf, return
 * ' '      → ignored
 *
 * LCD:
 *   Line1:  Enter Pass:
 *   Line2:  ****
 *=============================================================*/
void ReadPass(u8 *buf)
{
    u8 keyV;
    u8 len = 0;
    u8 i;
	delay_ms(20);

    for(i = 0; i <= PASS_LEN; i++)
  		buf[i] = '\0';

//    CmdLCD(CLEAR_LCD);
//    CmdLCD(GOTO_LINE1_POS0);
//    StrLCD((s8*)"Enter Pass:");
//    CmdLCD(GOTO_LINE2_POS0);

    while(1)
    {
        keyV = keyscan();
		delay_ms(100);

        /*--- Digit key -------------------------------------*/
        if(keyV >= '0' && keyV <= '9')
        {
            if(len < PASS_LEN)
            {
                buf[len] = keyV;    /* store real digit      */
                len++;

                /* show '*' at current column position       */
                CmdLCD(GOTO_LINE2_POS0 + (len - 1));
                CharLCD('*');
            }
            while(colscan() == 0);
        }

        /*--- Backspace '*' ---------------------------------*/
        else if(keyV == '*' )
        {
            if(len > 0)
            {
                len--;
                buf[len] = '\0';

                /* erase last '*' on LCD */
                CmdLCD(GOTO_LINE2_POS0 + len);
                CharLCD(' ');
                CmdLCD(GOTO_LINE2_POS0 + len); /* cursor back */
            }
            while(colscan() == 0);
        }

        /*--- Clear 'C' -------------------------------------*/
        else if(keyV == 'C')
        {
            for(i = 0; i < len; i++)
            {
                CmdLCD(GOTO_LINE2_POS0 + i);
                CharLCD(' ');
            }
            for(i = 0; i <= MAX_LEN; i++) buf[i] = '\0';
            len = 0;
            CmdLCD(GOTO_LINE2_POS0);

            while(colscan() == 0);
        }

        /*--- Enter '#' -------------------------------------*/
        else if((keyV == '#') && (len==PASS_LEN))
        {
            buf[len] = '\0';
            while(colscan() == 0);
            return;
        }

        /* ' ' ignored */
        else { while(colscan() == 0); }
    }
}
