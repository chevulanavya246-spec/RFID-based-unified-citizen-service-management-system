/*=============================================================
 * main_rfid.c
 * RFID-Based Unified Citizen Service Management System
 * LPC2148
 *
 * FIXES APPLIED:
 *  1. Init_SPI0() called AFTER UART0_Init() — spi.c now uses
 *     PINSEL0 |= (not =) so UART pins are NOT wiped
 *  2. First project initialization uses a dedicated MAGIC/VERSION
 *     signature, not 0xFF data bytes. This safely initializes a
 *     laboratory EEPROM that already contains old/garbage data.
 *  3. No vote_announce_result — removed completely
 *  4. Officer card goes to officer_menu() (vote reset + DL set)
 *=============================================================*/

#include <LPC21xx.h>
#include "types.h"
#include "defines.h"
#include "delay.h"
#include "lcd.h"
#include "lcd_defines.h"
#include "kpm1.h"
#include "kpm1_defines.h"
#include "rfid.h"
#include "rfid_defines.h"
#include "menu_rfid.h"
#include "menu_rfid_defines.h"
#include "spi.h"
#include "spi_eeprom.h"
#include "uart.h"
#include "rtc.h"

#define L1  GOTO_LINE1_POS0
#define L2  GOTO_LINE2_POS0
#define L3  GOTO_LINE3_POS0
#define L4  GOTO_LINE4_POS0

extern volatile u8 rfid_ready;

/*-------------------------------------------------------------
 * show_waiting() — idle screen shown while waiting for card
 *-------------------------------------------------------------*/
static void show_waiting(void)
{
    CmdLCD(CLEAR_LCD);
    delay_ms(2);
    CmdLCD(L1+2); StrLCD((s8*)"CITIZEN SERVICE");
    CmdLCD(L2+6); StrLCD((s8*)"SYSTEM");
    CmdLCD(L3+2); StrLCD((s8*)"SCAN YOUR CARD");
    CmdLCD(L4+2); StrLCD((s8*)"TO CONTINUE...");
    CmdLCD(DSP_ON_CUR_OFF);
}

/*=============================================================
 * Project EEPROM initialization guard
 *
 * DO NOT use a data byte such as password/balance/card ID to decide
 * whether the EEPROM is initialized. A laboratory EEPROM may contain
 * old or random data. These dedicated bytes identify THIS firmware's
 * EEPROM layout.
 *=============================================================*/
static u8 eeprom_project_is_initialized(void)
{
    if(ByteRead_25LC512(EEP_PROJECT_MAGIC_0) != 'R') return 0;
    if(ByteRead_25LC512(EEP_PROJECT_MAGIC_1) != 'F') return 0;
    if(ByteRead_25LC512(EEP_PROJECT_MAGIC_2) != 'I') return 0;
    if(ByteRead_25LC512(EEP_PROJECT_MAGIC_3) != 'D') return 0;
    if(ByteRead_25LC512(0x0006) != EEP_PROJECT_VERSION) return 0;
    return 1;
}

static void eeprom_project_mark_initialized(void)
{
    ByteWrite_25LC512(EEP_PROJECT_MAGIC_0, 'R');
    ByteWrite_25LC512(EEP_PROJECT_MAGIC_1, 'F');
    ByteWrite_25LC512(EEP_PROJECT_MAGIC_2, 'I');
    ByteWrite_25LC512(EEP_PROJECT_MAGIC_3, 'D');
    ByteWrite_25LC512(0x0006, EEP_PROJECT_VERSION);
    ByteWrite_25LC512(EEP_CARD_VERSION, CARD_VERSION);
}

static void rtc_save_to_eeprom(void)
{
    s32 h, m, sec, dd, mm, yy, dow;

    GetRTCTimeInfo(&h, &m, &sec);
    GetRTCDateInfo(&dd, &mm, &yy);
    GetRTCDay(&dow);

    ByteWrite_25LC512(EEP_RTC_HOUR,  (u8)h);
    ByteWrite_25LC512(EEP_RTC_MIN,   (u8)m);
    ByteWrite_25LC512(EEP_RTC_SEC,   (u8)sec);
    ByteWrite_25LC512(EEP_RTC_DAY,   (u8)dd);
    ByteWrite_25LC512(EEP_RTC_MONTH, (u8)mm);
    ByteWrite_25LC512(EEP_RTC_DOW,   (u8)dow);
    ByteWrite_25LC512(EEP_RTC_YEAR_H, (u8)(((u16)yy) >> 8));
    ByteWrite_25LC512(EEP_RTC_YEAR_L, (u8)((u16)yy));
}

static void rtc_restore_from_eeprom(void)
{
    u8 h, m, sec, dd, mm, dow, yh, yl;
    u16 yy;

    h   = ByteRead_25LC512(EEP_RTC_HOUR);
    m   = ByteRead_25LC512(EEP_RTC_MIN);
    sec = ByteRead_25LC512(EEP_RTC_SEC);
    dd  = ByteRead_25LC512(EEP_RTC_DAY);
    mm  = ByteRead_25LC512(EEP_RTC_MONTH);
    dow = ByteRead_25LC512(EEP_RTC_DOW);
    yh  = ByteRead_25LC512(EEP_RTC_YEAR_H);
    yl  = ByteRead_25LC512(EEP_RTC_YEAR_L);
    yy  = (u16)(((u16)yh << 8) | yl);

    SetRTCTime(h, m, sec);
    SetRTCDate(dd, mm, yy);
    SetRTCDay(dow);
}

	u8 EEPROM_Test(void)
	{
		u8 old_value;
		u8 read_value;

		old_value=ByteRead_25LC512(EEP_TEST_ADDR);
		ByteWrite_25LC512(EEP_TEST_ADDR,EEP_TEST_VALUE);
		delay_ms(10);

		read_value=ByteRead_25LC512(EEP_TEST_ADDR);
		ByteWrite_25LC512(EEP_TEST_ADDR,old_value);
		delay_ms(10);

		if(read_value == EEP_TEST_VALUE)
			return 1;

		return 0;

	}


/*=============================================================
 * main()
 *=============================================================*/
int main(void)
{
    u8   card_buf[9];
    s8   user;
    s8  *uname;

    /*--- Init hardware in correct order --------------------
     * UART0_Init() must come BEFORE Init_SPI0().
     * spi.c now uses PINSEL0 |= so UART pins stay intact.
     *------------------------------------------------------*/
    UART0_Init();
    Init_SPI0();
    InitLCD();
    InitKPM();
    rfid_gpio_init();
    RTC_Init();

    /*--- Splash screen -------------------------------------*/
    CmdLCD(CLEAR_LCD); delay_ms(2);
    CmdLCD(L1); StrLCD((s8*)"  RFID CITIZEN  ");
    CmdLCD(L2); StrLCD((s8*)" SERVICE SYSTEM ");
    CmdLCD(L3); StrLCD((s8*)"  LPC2148 Based ");
    CmdLCD(DSP_ON_CUR_OFF);
    delay_ms(2000);

	if(EEPROM_Test() == 0)
	{
		CmdLCD(0x01);
		CmdLCD(L1);
		StrLCD("EEPROM ERROR:");

		CmdLCD(L2);
		StrLCD("NOT CONNECTED...");

		while(1);
	}	

    /*--- First project initialization ----------------------
     * A lab EEPROM can already contain data. Therefore we DO NOT
     * test for 0xFF. We test a dedicated project signature/version.
     * If it is absent, ALL project defaults are deliberately written,
     * including the password, balances, votes, DL values, cards and RTC.
     * Once the marker is valid, these defaults are never written again.
     *------------------------------------------------------*/
    if(!eeprom_project_is_initialized())
    {
        CmdLCD(CLEAR_LCD); delay_ms(2);
        CmdLCD(L1); StrLCD((s8*)"First Time Setup");
        CmdLCD(L2); StrLCD((s8*)"Writing EEPROM..");
        CmdLCD(DSP_ON_CUR_OFF);
        delay_ms(1500);

        /* Write ALL first-project defaults, regardless of old EEPROM data. */
        rfid_store_cards();
        menu_init_eeprom();	   

        /* Initial RTC values for this project. */
        SetRTCTime(12,40,0);
        SetRTCDate(7,8,2026);
        SetRTCDay(5);
        rtc_save_to_eeprom();

        /* Mark only AFTER every required item has been written. */
        eeprom_project_mark_initialized();

        CmdLCD(CLEAR_LCD); delay_ms(2);
        CmdLCD(L1); StrLCD((s8*)"Setup Complete!");
        delay_ms(1500);
    }
    else
    {
        /* Restore the last project-saved RTC value. */
        rtc_restore_from_eeprom();
    }

    /* RFID card database update.
     * Change CARD_VERSION when the configured card IDs change.
     * This updates ONLY card IDs; ATM/password/vote/DL/RTC data is preserved. */
    if(ByteRead_25LC512(EEP_CARD_VERSION) != CARD_VERSION)
    {
        rfid_store_cards();
        ByteWrite_25LC512(EEP_CARD_VERSION, CARD_VERSION);
    }

  
    /*--- Idle screen --------------------------------------*/
    show_waiting();	   

    /*--- Main loop -----------------------------------------*/
    while(1)
    {
        /* Block here until full 10-byte RFID packet arrives */
			  rfid_ready=0;
        rfid_read_card(card_buf);	 

        /* Compare received card against EEPROM stored IDs */
        user = rfid_get_user(card_buf);	

        /*--- Invalid card ----------------------------------*/
        if(user == INVALID_CARD)
        {
            rfid_invalid_indication();
            CmdLCD(CLEAR_LCD); delay_ms(2);
            CmdLCD(L1+1); StrLCD((s8*)"Invalid Card!");
            CmdLCD(L2+1); StrLCD((s8*)"Access Denied.");
            CmdLCD(DSP_ON_CUR_OFF);
            delay_ms(1500);
            show_waiting();
        }

        /*--- Officer card ----------------------------------*/
        else if(user == OFFICER)
        {
            rfid_valid_indication();
            CmdLCD(CLEAR_LCD); delay_ms(2);
            CmdLCD(L1+3); StrLCD((s8*)"Officer Card");
            CmdLCD(L2+4); StrLCD((s8*)"Welcome!");
            CmdLCD(DSP_ON_CUR_OFF);
            delay_ms(1000);

            /* 2-option officer menu:
             * 1 = Reset all votes
             * 2 = Set DL expiry (RTC year + 20) */
            officer_menu();

            show_waiting();
        }

        /*--- Valid user card — show service menu -----------*/
        else
        {
            rfid_valid_indication();

            if(user == USER1)       uname = (s8*)PAN_NAME_U1;
            else if(user == USER2)  uname = (s8*)PAN_NAME_U2;
            else                    uname = (s8*)PAN_NAME_U3;

            CmdLCD(CLEAR_LCD); delay_ms(2);
            CmdLCD(L1+2); StrLCD((s8*)"Valid Card!");
            CmdLCD(L2+4); StrLCD((s8*)"Welcome");
            CmdLCD(L3+2); StrLCD(uname);
            CmdLCD(DSP_ON_CUR_OFF);
            delay_ms(2000);

            /* Enter service menu for this user. */
            if(show_menu(user) == PASS_LOCKED)
            {
                /* Three wrong passwords end this card session immediately. */
                show_waiting();
                continue;
            }

            /* Normal menu exit also returns to the card-waiting screen. */
            show_waiting();
        }
    }
}
