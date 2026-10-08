/*=============================================================
 * menu_rfid.c
 * Unified Citizen Service Management System
 *
 * Changes from original:
 *  1. AUTO-TIMEOUT: every screen auto-exits after 10 seconds
 *     if no key pressed. '*' key exits any screen immediately.
 *  2. ATM: single txn limit Rs.45000, no daily limit.
 *     Balance saved to EEPROM on every transaction.
 *     Balance enquiry always reads fresh from EEPROM.
 *  3. DL: expiry year read from EEPROM, compared with RTC YEAR.
 *     Real RTC date+time shown on LCD.
 *  4. OFFICER card: 2-option menu
 *       1 = Reset all votes (party counts + user flags)
 *       2 = Set DL expiry year for any user via keypad
 *     No vote announcement.
 *=============================================================*/

#include <LPC21xx.h>
#include "types.h"
#include "defines.h"
#include "delay.h"
#include "lcd.h"
#include "lcd_defines.h"
#include "kpm1.h"
#include "kpm1_defines.h"
#include "rfid_defines.h"
#include "menu_rfid_defines.h"
#include "menu_rfid.h"
#include "spi_eeprom.h"
#include "rtc.h"

/*--- LCD line macros ----------------------------------------*/
#define L1  GOTO_LINE1_POS0
#define L2  GOTO_LINE2_POS0
#define L3  GOTO_LINE3_POS0
#define L4  GOTO_LINE4_POS0

/*=============================================================
 * clr() — clear display
 *=============================================================*/
static void clr(void)
{
    CmdLCD(CLEAR_LCD);
    delay_ms(2);
    CmdLCD(DSP_ON_CUR_OFF);
}

/*=============================================================
 * timed_wait_key(seconds)
 *
 * Waits up to 'seconds' for a key press.
 * Shows countdown on LCD LINE4 col 18-19 (rightmost):
 *   10→09→08→...→01 each decreasing every second.
 * Returns key pressed, or 0xFF if timeout.
 *=============================================================*/
/* The current screen tells timed_wait_key which keypad keys are valid. */
static const s8 *wait_valid_keys = (const s8 *)"*";

/* Set the valid keys for the next timeout screen. */
static void set_wait_valid_keys(const s8 *keys)
{
    wait_valid_keys = keys;
}

/* Return 1 only when the key belongs to the current screen. */
static u8 wait_key_is_valid(u8 key)
{
    u8 i = 0;
    while(wait_valid_keys[i] != '\0')
    {
        if((u8)wait_valid_keys[i] == key)
            return 1;
        i++;
    }
    return 0;
}

/*=============================================================
 * timed_wait_key(seconds)
 *
 * Invalid/unwanted keys are consumed and ignored here.
 * They never return to the menu, so the same countdown continues
 * and the LCD is not cleared or restarted because of that key.
 *=============================================================*/
static u8 timed_wait_key(u32 seconds)
{
    u32 s, i;
    u8  k;

    for(s = seconds; s > 0; s--)
    {
        /* Update only the countdown field once per second. */
        CmdLCD(L4 + 18);
        if(s < 10)
        {
            CharLCD('0');
            CharLCD((u8)('0' + s));
        }
        else
        {
            CharLCD((u8)('0' + s/10));
            CharLCD((u8)('0' + s%10));
        }

        for(i = 0; i < 10; i++)
        {
            if(colscan() == 0)
            {
                k = keyscan();
                while(colscan() == 0);

                /* Valid key exits this wait; invalid key is ignored. */
                if(wait_key_is_valid(k))
                    return k;
            }
            delay_ms(100);
        }
    }
    return 0xFF;
}

/*=============================================================
 * PASSWORD — EEPROM backed
 *=============================================================*/

/*-------------------------------------------------------------
 * pass_read_eeprom(buf)
 * Reads 4 password digit bytes from EEPROM into buf[0..3].
 * buf must be PASS_LEN+1 bytes.
 *-------------------------------------------------------------*/
void pass_read_eeprom(u8 *buf)
{
    buf[0] = ByteRead_25LC512(EEP_PASS_0);
    buf[1] = ByteRead_25LC512(EEP_PASS_1);
    buf[2] = ByteRead_25LC512(EEP_PASS_2);
    buf[3] = ByteRead_25LC512(EEP_PASS_3);
    buf[4] = '\0';
}

/*-------------------------------------------------------------
 * pass_write_eeprom(buf)
 * Writes 4 password digit bytes from buf into EEPROM.
 *-------------------------------------------------------------*/
void pass_write_eeprom(u8 *buf)
{
    ByteWrite_25LC512(EEP_PASS_0, buf[0]);
    ByteWrite_25LC512(EEP_PASS_1, buf[1]);
    ByteWrite_25LC512(EEP_PASS_2, buf[2]);
    ByteWrite_25LC512(EEP_PASS_3, buf[3]);
}

/*-------------------------------------------------------------
 * pass_ok()
 * 3-attempt password check.
 * Shows "Enter Password:" on LCD.
 * Each wrong attempt shows "X chances left".
 * After 3 wrong attempts shows "Locked!" and returns 0.
 * '*' key at any time cancels and returns 0.
 * Returns 1 if correct, 0 if wrong/locked/cancelled.
 *-------------------------------------------------------------*/
static s8 pass_ok(void)
{
    u8 entered[PASS_LEN + 1];
    u8 stored [PASS_LEN + 1];
    u8 i;
    u8 attempts = 0;
    u8 match;

    /* Read current password from EEPROM once */
    pass_read_eeprom(stored);

    while(attempts < 3)
    {
        /* Show attempt prompt */
        clr();
        CmdLCD(L1); StrLCD((s8*)"Enter Password:");
        CmdLCD(L2); StrLCD((s8*)"(# confirm *=Back)");
        if(attempts > 0)
        {
            CmdLCD(L3);
            CharLCD('0' + (3 - attempts));
            StrLCD((s8*)" chances left");
        }

        /* Get password — ReadPass clears line2 and shows **** */
		CmdLCD(CLEAR_LCD);
    	CmdLCD(GOTO_LINE1_POS0);
   		StrLCD((s8*)"Enter Pass:");
	    CmdLCD(GOTO_LINE2_POS0);
        ReadPass(entered);

        /* * key returns empty buf — treat as cancel */
        if(entered[0] == '\0')
        {
            clr();
            CmdLCD(L1); StrLCD((s8*)"Cancelled.");
            delay_ms(1000);
            return PASS_FAIL;
        }

        /* Compare entered vs EEPROM */
        match = 1;
        for(i = 0; i < PASS_LEN; i++)
            if(entered[i] != stored[i]) { match = 0; break; }

        if(match)
            return PASS_OK;   /* correct password */

        /* Wrong password */
        attempts++;
        clr();
        CmdLCD(L1); StrLCD((s8*)"Wrong Password!");
        if(attempts < 3)
        {
            CmdLCD(L2);
            CharLCD('0' + (3 - attempts));
            StrLCD((s8*)" chances left");
        }
        delay_ms(1500);
    }

    /* All 3 attempts failed */
    clr();
    CmdLCD(L1); StrLCD((s8*)"ACCESS LOCKED!");
    CmdLCD(L2); StrLCD((s8*)"3 wrong attempts");
    CmdLCD(L3); StrLCD((s8*)"Swipe card again");
    delay_ms(2500);
    return PASS_LOCKED;
}

/*=============================================================
 * menu_change_password()
 *
 * Full password change flow:
 *   Step 1: Enter current password (verified against EEPROM)
 *   Step 2: Enter new password
 *   Step 3: Confirm new password (must match step 2)
 *   If all match → write new password to EEPROM
 *
 * LCD flow:
 *   "Current Password:"  → ****
 *   "New Password:"      → ****
 *   "Confirm Password:"  → ****
 *   "Password Changed!"  or  "Mismatch! Try again"
 *
 * '*' exits at any step. Auto-timeout = 10s per step.
 *=============================================================*/
s8 menu_change_password(void)
{
    u8 current [PASS_LEN + 1];
    u8 stored  [PASS_LEN + 1];
    u8 newpass [PASS_LEN + 1];
    u8 confirm [PASS_LEN + 1];
    u8 i;
    u8 match;

    /*--- Step 1: Verify current password -------------------*/
    pass_read_eeprom(stored);

    clr();
//    CmdLCD(L1); StrLCD((s8*)"Change Password");
    CmdLCD(L1); StrLCD((s8*)"Current Password:");
	CmdLCD(L4); StrLCD((s8*)"(# confirm )");
	CmdLCD(L2);ReadPass(current);
    delay_ms(2000);

//    clr();
//    CmdLCD(L1); StrLCD((s8*)"Current Password:");


    /* Check if '*' pressed (empty buf — all '\0') */
    if(current[0] == '\0')
    {
        clr();
        CmdLCD(L1); StrLCD((s8*)"Cancelled.");
        delay_ms(1500);
        return PASS_FAIL;
    }

    /* Verify current password matches EEPROM */
    match = 1;
    for(i = 0; i < PASS_LEN; i++)
	{
        if(current[i] != stored[i]) 
		{ 
			match = 0; 
			break; 
		}
	}

    if(!match)
    {
        clr();
        CmdLCD(L1); StrLCD((s8*)"Wrong Password!");
        CmdLCD(L2); StrLCD((s8*)"Cannot change.");
        delay_ms(1500);
        return PASS_FAIL;
    }

    /*--- Step 2: Enter new password ------------------------*/
    clr();
    CmdLCD(L1); StrLCD((s8*)"Set New Password:");
	CmdLCD(L4); StrLCD((s8*)"(# confirm )");
	CmdLCD(L2); ReadPass(newpass);
   
    delay_ms(1000);

    if(newpass[0] == '\0')
    {
        clr();
        CmdLCD(L1); StrLCD((s8*)"Cancelled.");
        delay_ms(1000);
        return PASS_FAIL;
    }

    /*--- Step 3: Confirm new password ----------------------*/
    clr();
    CmdLCD(L1); StrLCD((s8*)"Confirm Password:");
	CmdLCD(L4); StrLCD((s8*)"(# confirm )");
	CmdLCD(L2); ReadPass(confirm);
    

    delay_ms(1000);

    if(confirm[0] == '\0')
    {
        clr();
        CmdLCD(L1); StrLCD((s8*)"Cancelled.");
        delay_ms(1000);
        return PASS_FAIL;
    }

    /*--- Compare new and confirm ---------------------------*/
    match = 1;
    for(i = 0; i < PASS_LEN; i++)
        if(newpass[i] != confirm[i]) { match = 0; break; }

    if(!match)
    {
        clr();
        CmdLCD(L1); StrLCD((s8*)"Passwords do not");
        CmdLCD(L2); StrLCD((s8*)"match! Try again.");
        delay_ms(2000);
        return PASS_FAIL;
    }

    /*--- Save new password to EEPROM -----------------------*/
    pass_write_eeprom(newpass);

    clr();
    CmdLCD(L1); StrLCD((s8*)"Password Changed!");
    CmdLCD(L2); StrLCD((s8*)"Successfully.");
    CmdLCD(L3); StrLCD((s8*)"New pass saved ");
 //   CmdLCD(L4); StrLCD((s8*)"EEPROM. Power safe");
    delay_ms(2500);
    return PASS_OK;
}

/*=============================================================
 * EEPROM HELPERS
 *=============================================================*/

/*--- Balance (u32 — no upper limit, stored as 4 bytes) ------*
 * Layout in EEPROM per user (4 bytes):
 *   addr+0 = byte3 (MSB), addr+1 = byte2,
 *   addr+2 = byte1,       addr+3 = byte0 (LSB)
 * Reuses existing H/L addresses as start of 4-byte block:
 *   User1: 0x0030-0x0033
 *   User2: 0x0034-0x0037   (shifted — see new EEPROM layout)
 *   User3: 0x0038-0x003B
 *
 * NOTE: EEPROM vote addresses also shift accordingly.
 * See menu_rfid_defines.h updated map.
 *-------------------------------------------------------------*/
static u16 bal_base(s8 user)
{
    if(user == USER1) return EEP_BAL_H_U1;
    if(user == USER2) return EEP_BAL_H_U2;
    return EEP_BAL_H_U3;
}

u32 eep_get_balance(s8 user)
{
    u16 base = bal_base(user);
    u32 val;
    val  = (u32)ByteRead_25LC512(base)     << 24;
    val |= (u32)ByteRead_25LC512(base + 1) << 16;
    val |= (u32)ByteRead_25LC512(base + 2) << 8;
    val |= (u32)ByteRead_25LC512(base + 3);
    return val;
}

void eep_set_balance(s8 user, u32 amount)
{
    u16 base = bal_base(user);
    ByteWrite_25LC512(base,     (u8)((amount >> 24) & 0xFF));
    ByteWrite_25LC512(base + 1, (u8)((amount >> 16) & 0xFF));
    ByteWrite_25LC512(base + 2, (u8)((amount >> 8)  & 0xFF));
    ByteWrite_25LC512(base + 3, (u8)( amount         & 0xFF));
}

/*--- Vote flag ----------------------------------------------*/
static u16 vote_addr(s8 user)
{
    if(user == USER1) return EEP_VOTE_U1;
    if(user == USER2) return EEP_VOTE_U2;
    return EEP_VOTE_U3;
}

u8 eep_get_vote(s8 user)
{
    return ByteRead_25LC512(vote_addr(user));
}

void eep_set_vote(s8 user, u8 status)
{
    ByteWrite_25LC512(vote_addr(user), status);
}

/*--- DL expiry full date (DD, MM, YYYY) — 4 bytes per user --*/
static void dl_addr(s8 user, u16 *dd, u16 *mm, u16 *yh, u16 *yl)
{
    if(user == USER1)
    { *dd=EEP_DL_DD_U1; *mm=EEP_DL_MM_U1; *yh=EEP_DL_YH_U1; *yl=EEP_DL_YL_U1; }
    else if(user == USER2)
    { *dd=EEP_DL_DD_U2; *mm=EEP_DL_MM_U2; *yh=EEP_DL_YH_U2; *yl=EEP_DL_YL_U2; }
    else
    { *dd=EEP_DL_DD_U3; *mm=EEP_DL_MM_U3; *yh=EEP_DL_YH_U3; *yl=EEP_DL_YL_U3; }
}

/* Write full expiry date DD/MM/YYYY to EEPROM */
static void eep_set_dl_expiry(s8 user, u8 day, u8 month, u16 year)
{
    u16 dd, mm, yh, yl;
    dl_addr(user, &dd, &mm, &yh, &yl);
    ByteWrite_25LC512(dd, day);
    ByteWrite_25LC512(mm, month);
    ByteWrite_25LC512(yh, (u8)(year >> 8));
    ByteWrite_25LC512(yl, (u8)(year & 0xFF));
}

/* Read full expiry date from EEPROM into separate variables */
static void eep_get_dl_expiry(s8 user, u8 *day, u8 *month, u16 *year)
{
    u16 dd, mm, yh, yl;
    dl_addr(user, &dd, &mm, &yh, &yl);
    *day   = ByteRead_25LC512(dd);
    *month = ByteRead_25LC512(mm);
    *year  = (u16)((ByteRead_25LC512(yh) << 8) | ByteRead_25LC512(yl));
}

/*=============================================================
 * menu_init_eeprom()
 * Call ONCE on first flash then comment out.
 *=============================================================*/
void menu_init_eeprom(void)
{
    u8 i;
    const char *def = DEFAULT_PASSWORD;

    /* Project first initialization: overwrite ALL project defaults.
     * This intentionally ignores whatever old/garbage bytes are in
     * the laboratory EEPROM. This function must only be called when
     * the project MAGIC/VERSION check says this EEPROM is new. */
    eep_set_balance(USER1, INIT_BALANCE);
    eep_set_balance(USER2, INIT_BALANCE);
    eep_set_balance(USER3, INIT_BALANCE);

    eep_set_vote(USER1, NOT_VOTED);
    eep_set_vote(USER2, NOT_VOTED);
    eep_set_vote(USER3, NOT_VOTED);

    ByteWrite_25LC512(EEP_CNT_A,    0);
    ByteWrite_25LC512(EEP_CNT_B,    0);
    ByteWrite_25LC512(EEP_CNT_C,    0);
    ByteWrite_25LC512(EEP_CNT_D,    0);
    ByteWrite_25LC512(EEP_CNT_NOTA, 0);

    eep_set_dl_expiry(USER1, DL_EXPIRY_DD, DL_EXPIRY_MM, (u16)DL_EXPIRY_U1);
    eep_set_dl_expiry(USER2, DL_EXPIRY_DD, DL_EXPIRY_MM, (u16)DL_EXPIRY_U2);
    eep_set_dl_expiry(USER3, DL_EXPIRY_DD, DL_EXPIRY_MM, (u16)DL_EXPIRY_U3);

    /* IMPORTANT: first project initialization always writes the
     * configured password, even if the lab EEPROM already contains
     * some other value at 0x0060-0x0063. */
    for(i = 0; i < PASS_LEN; i++)
        ByteWrite_25LC512((u16)(EEP_PASS_0 + i), (u8)def[i]);
}


/*=============================================================
 * CGRAM symbols for voting (5 parties)
 *=============================================================*/
static u8 vote_symbols[40] = {
    0x00,0x04,0x0E,0x1F,0x0E,0x04,0x04,0x00, /* 0: Lotus A  */
    0x0A,0x0A,0x1E,0x0F,0x0E,0x0E,0x04,0x00, /* 1: Hand  B  */
    0x00,0x04,0x06,0x0D,0x16,0x0A,0x0A,0x00, /* 2: Cycle C  */
    0x01,0x02,0x04,0x08,0x10,0x1F,0x0E,0x04, /* 3: Broom D  */
    0x00,0x11,0x0A,0x04,0x0A,0x11,0x00,0x1F  /* 4: NOTA X   */
};

static void vote_load_symbols(void)
{
    BuildCGRAM(vote_symbols, 40);
}

static void eep_inc_party(u16 addr)
{
    u8 cnt = ByteRead_25LC512(addr);
    cnt++;
    ByteWrite_25LC512(addr, cnt);
}

/*=============================================================
 * vote_reset_counts()
 * Zero all 5 party counts + all 3 user vote flags.
 *=============================================================*/
void vote_reset_counts(void)
{
    ByteWrite_25LC512(EEP_CNT_A,    0);
    ByteWrite_25LC512(EEP_CNT_B,    0);
    ByteWrite_25LC512(EEP_CNT_C,    0);
    ByteWrite_25LC512(EEP_CNT_D,    0);
    ByteWrite_25LC512(EEP_CNT_NOTA, 0);
    eep_set_vote(USER1, NOT_VOTED);
    eep_set_vote(USER2, NOT_VOTED);
    eep_set_vote(USER3, NOT_VOTED);
}

/*=============================================================
 * officer_menu()
 * Called when OFFICER card is swiped.
 * Two options:
 *   1 = Reset all votes (party counts + user flags)
 *   2 = Set DL expiry year for a user (enter via keypad)
 * '*' or timeout exits back to idle.
 *=============================================================*/
void officer_menu(void)
{
    u8  key;

    while(1)
    {
        clr();
        CmdLCD(L1); StrLCD((s8*)"-- OFFICER MENU --");
        CmdLCD(L2); StrLCD((s8*)"1-Reset Votes");
        CmdLCD(L3); StrLCD((s8*)"2-Set DL Expiry");
        CmdLCD(L4); StrLCD((s8*)"* Exit");

        set_wait_valid_keys((s8*)"12*");
        key = timed_wait_key(AUTO_TIMEOUT_S);

        /*--- 1. Reset all votes ----------------------------*/
        if(key == '1')
        {
            vote_reset_counts();
            clr();
            CmdLCD(L1); StrLCD((s8*)"All Votes RESET!");
            CmdLCD(L2); StrLCD((s8*)"Party counts = 0");
 //           CmdLCD(L3); StrLCD((s8*)"User flags cleared");
            delay_ms(2000);
        }

        /*--- 2. Set DL expiry = RTC current date + 20 years --*/
        else if(key == '2')
        {
            s32 rtc_date, rtc_month, rtc_year;
            u8  user_key;
            s8  sel_user;
			u16 new_year;

            /* Read current RTC date */
            GetRTCDateInfo(&rtc_date, &rtc_month, &rtc_year);
            new_year = (u16)(rtc_year + 20);

            /* Ask which user */
            clr();
            CmdLCD(L1); StrLCD((s8*)"Set DL Expiry");
            CmdLCD(L2); StrLCD((s8*)"1-Mani Teja");
            CmdLCD(L3); StrLCD((s8*)"2-Santhos Pawar");
            CmdLCD(L4); StrLCD((s8*)"3-Guru Prasad *=Bk");

            set_wait_valid_keys((s8*)"123*");
            user_key = timed_wait_key(AUTO_TIMEOUT_S);
            if(user_key == '*') continue;
            if(user_key < '1' || user_key > '3') continue;

            sel_user = (s8)(user_key - '1');   /* 0, 1, or 2  */

            /* Read the existing expiry before changing anything. */
            {
                u8 old_dd, old_mm;
                u16 old_year;
                u8 expired;

                eep_get_dl_expiry(sel_user, &old_dd, &old_mm, &old_year);

                /* Full date comparison: update only when already expired. */
                expired = 0;
                if((u32)rtc_year > (u32)old_year ||
                   ((u32)rtc_year == (u32)old_year && (u32)rtc_month > (u32)old_mm) ||
                   ((u32)rtc_year == (u32)old_year && (u32)rtc_month == (u32)old_mm &&
                    (u32)rtc_date > (u32)old_dd))
                    expired = 1;

                if(expired)
                {
                    /* Expired DL: reset it to current DD/MM + 20 years. */
                    eep_set_dl_expiry(sel_user,
                                      (u8)rtc_date,
                                      (u8)rtc_month,
                                      new_year);

                    clr();
                    CmdLCD(L1); StrLCD((s8*)"DL Expiry Updated!");
                    CmdLCD(L2);
                    if(sel_user==USER1)      
						StrLCD((s8*)"Mani Teja");
                    else if(sel_user==USER2) 
						StrLCD((s8*)"Santhos Pawar");
                    else                     
						StrLCD((s8*)"Guru Prasad");

                    CmdLCD(L3); StrLCD((s8*)"New exp: ");
                    CharLCD((u8)(rtc_date/10) + '0');
                    CharLCD((u8)(rtc_date%10) + '0');
                    CharLCD('/');
                    CharLCD((u8)(rtc_month/10) + '0');
                    CharLCD((u8)(rtc_month%10) + '0');
                    CharLCD('/');
                    U32LCD((u32)new_year);
                    CmdLCD(L4); StrLCD((s8*)"Expired -> Reset");
                }
                else
                {
                    /* Still valid: keep the old EEPROM expiry unchanged. */
                    clr();
                    CmdLCD(L1); StrLCD((s8*)"DL Still Valid");
                    CmdLCD(L2); StrLCD((s8*)"No Change Made");
                    CmdLCD(L3); StrLCD((s8*)"Old expiry: ");
					CmdLCD(L4);
                    CharLCD((u8)(old_dd/10) + '0');
                    CharLCD((u8)(old_dd%10) + '0');
                    CharLCD('/');
                    CharLCD((u8)(old_mm/10) + '0');
                    CharLCD((u8)(old_mm%10) + '0');
                    CharLCD('/');
                    U32LCD((u32)old_year);
                }
                delay_ms(2500);
            }
        }

        /*--- * or timeout — exit ---------------------------*/
        else
        {
            return;
        }
    }
}

/*=============================================================
 * menu_pan(user)
 * Option 1 — PAN Card
 * Password protected. Shows Name, DOB, PAN number.
 * '*' exits immediately. Auto-timeout after 10 seconds.
 *=============================================================*/
s8 menu_pan(s8 user)
{
    s8 *name, *dob, *pan;
    u8  key;

    if(user == USER1)
    { name=(s8*)PAN_NAME_U1; dob=(s8*)PAN_DOB_U1; pan=(s8*)PAN_NUM_U1; }
    else if(user == USER2)
    { name=(s8*)PAN_NAME_U2; dob=(s8*)PAN_DOB_U2; pan=(s8*)PAN_NUM_U2; }
    else
    { name=(s8*)PAN_NAME_U3; dob=(s8*)PAN_DOB_U3; pan=(s8*)PAN_NUM_U3; }

    {
        s8 pass_result = pass_ok();
        if(pass_result == PASS_LOCKED)
            return PASS_LOCKED;
        if(pass_result != PASS_OK)
            return PASS_FAIL;
    }

    /* Show PAN details */
    clr();
    CmdLCD(L1); StrLCD((s8*)"Name :"); StrLCD(name);
    CmdLCD(L2); StrLCD((s8*)"DOB  :"); StrLCD(dob);
    CmdLCD(L3); StrLCD((s8*)"PAN  :"); StrLCD(pan);
    CmdLCD(L4); StrLCD((s8*)"* Exit ");

	while(1)
	{
    	/* Wait only for '*' on the detail screen; other keys are ignored. */
        set_wait_valid_keys((s8*)"*");
    	key = timed_wait_key(AUTO_TIMEOUT_S);

    	/* '*' or timeout — return to menu */
    	if(key== '*')
			break;
		else if(key == 0xff)
			break;
	}
    return PASS_FAIL;
}

/*=============================================================
 * menu_atm(user)
 * Option 2 — ATM
 *
 * Rules:
 *   Notes : Rs.100 / Rs.500 only (multiple of 100)
 *   Withdraw: min Rs.100, max Rs.45000 per txn
 *   Deposit : min Rs.100, max Rs.45000 per txn
 *   Balance must stay >= Rs.500 after withdraw
 *   Max balance: Rs.45000
 *   Balance saved to EEPROM immediately — power-safe
 *   '*' exits any sub-screen. Auto-timeout = 10s.
 *=============================================================*/
s8 menu_atm(s8 user)
{
    u8  key;
    u32 bal, amount;        /* u32 — balance can be lakhs     */

    {
        s8 pass_result = pass_ok();
        if(pass_result == PASS_LOCKED)
            return PASS_LOCKED;
        if(pass_result != PASS_OK)
            return PASS_FAIL;
    }

    while(1)
    {
        clr();
        CmdLCD(L1); StrLCD((s8*)"--- ATM MENU ---");
        CmdLCD(L2); StrLCD((s8*)"1-Bal  2-Withdraw");
        CmdLCD(L3); StrLCD((s8*)"3-Deposit ");
        CmdLCD(L4); StrLCD((s8*)"*=Back ");

        set_wait_valid_keys((s8*)"123*");
        key = timed_wait_key(AUTO_TIMEOUT_S);

        /*--- 1. Balance enquiry — always fresh from EEPROM -*/
        if(key == ATM_BALANCE)
        {
            bal = eep_get_balance(user);
            clr();
            CmdLCD(L1); StrLCD((s8*)"Account Balance:");
            CmdLCD(L2); StrLCD((s8*)"Rs. "); U32LCD(bal);
           // CmdLCD(L3); StrLCD((s8*)"Min keep: Rs.500");
            CmdLCD(L4); StrLCD((s8*)"*=Back ");
            set_wait_valid_keys((s8*)"*");
            timed_wait_key(AUTO_TIMEOUT_S);
        }

        /*--- 2. Withdrawal ---------------------------------*/
        else if(key == ATM_WITHDRAW)
        {
            bal = eep_get_balance(user);

            clr();
            CmdLCD(L1); StrLCD((s8*)"--- WITHDRAW ---");
            CmdLCD(L2); StrLCD((s8*)"Min:100 Max:45000");
            CmdLCD(L3); StrLCD((s8*)"100/500 notes only");
            CmdLCD(L4); StrLCD((s8*)"#=OK  *=Cancel");
            delay_ms(2000);

            clr();
            CmdLCD(L1); StrLCD((s8*)"Enter Amount:");
            CmdLCD(L4); StrLCD((s8*)"#=OK  *=Cancel");
            amount = ReadNum();

            if(amount == 0)
            { /* nothing entered — back to ATM menu */ }

            else if((amount % NOTE_MULTIPLE) != 0)
            {
                clr();
                CmdLCD(L1); StrLCD((s8*)"Invalid Amount!");
                CmdLCD(L2); StrLCD((s8*)"100 or 500 notes");
                CmdLCD(L3); StrLCD((s8*)"only. Try again.");
                delay_ms(2000);
            }
            else if(amount < MIN_WITHDRAW)
            {
                clr();
                CmdLCD(L1); StrLCD((s8*)"Min Withdraw:");
                CmdLCD(L2); StrLCD((s8*)"Rs.100");
                delay_ms(2000);
            }
            else if(amount > MAX_WITHDRAW)
            {
                clr();
                CmdLCD(L1); StrLCD((s8*)"Max per txn:");
                CmdLCD(L2); StrLCD((s8*)"Rs.45000 only");
                delay_ms(2000);
            }
            else if(amount > bal)
            {
                clr();
                CmdLCD(L1); StrLCD((s8*)"Insufficient Bal!");
                CmdLCD(L2); StrLCD((s8*)"Balance: Rs.");
                U32LCD(bal);
                delay_ms(2000);
            }
            else if((bal - amount) < MIN_BALANCE)
            {
                clr();
                CmdLCD(L1); StrLCD((s8*)"Cannot Withdraw!");
                CmdLCD(L2); StrLCD((s8*)"Min Rs.500 must");
                CmdLCD(L3); StrLCD((s8*)"stay in account.");
                delay_ms(2000);
            }
            else
            {
                bal -= amount;
                eep_set_balance(user, bal);
                clr();
                CmdLCD(L1); StrLCD((s8*)"Withdraw OK!");
                CmdLCD(L2); StrLCD((s8*)"Cash: Rs."); U32LCD(amount);
                CmdLCD(L3); StrLCD((s8*)"Bal : Rs."); U32LCD(bal);
                CmdLCD(L4); StrLCD((s8*)"Please take cash");
                delay_ms(2500);
            }
        }

        /*--- 3. Deposit ------------------------------------*/
        else if(key == ATM_DEPOSIT)
        {
            bal = eep_get_balance(user);

            clr();
            CmdLCD(L1); StrLCD((s8*)"--- DEPOSIT ---");
            CmdLCD(L2); StrLCD((s8*)"Min:100 Max:45000");
            CmdLCD(L3); StrLCD((s8*)"100/500 notes only");
            CmdLCD(L4); StrLCD((s8*)"#=OK  *=Cancel");
            delay_ms(2000);

            clr();
            CmdLCD(L1); StrLCD((s8*)"Enter Amount:");
            CmdLCD(L4); StrLCD((s8*)"#=OK  *=Cancel");
            amount = ReadNum();

            if(amount == 0)
            { /* nothing entered */ }

            else if((amount % NOTE_MULTIPLE) != 0)
            {
                clr();
                CmdLCD(L1); StrLCD((s8*)"Invalid Amount!");
                CmdLCD(L2); StrLCD((s8*)"100 or 500 notes");
                CmdLCD(L3); StrLCD((s8*)"only. Try again.");
                delay_ms(2000);
            }
            else if(amount < MIN_DEPOSIT)
            {
                clr();
                CmdLCD(L1); StrLCD((s8*)"Min Deposit:");
                CmdLCD(L2); StrLCD((s8*)"Rs.100");
                delay_ms(2000);
            }
            else if(amount > MAX_DEPOSIT)
            {
                clr();
                CmdLCD(L1); StrLCD((s8*)"Max per txn:");
                CmdLCD(L2); StrLCD((s8*)"Rs.45000 only");
                delay_ms(2000);
            }
            else
            {
                /* No balance ceiling — deposit always allowed */
                bal += amount;
                eep_set_balance(user, bal);
                clr();
                CmdLCD(L1); StrLCD((s8*)"Deposit OK!");
                CmdLCD(L2); StrLCD((s8*)"Amt : Rs."); U32LCD(amount);
                CmdLCD(L3); StrLCD((s8*)"Bal : Rs."); U32LCD(bal);
                delay_ms(2500);
            }
        }

        /*--- 4. Back / '*' / timeout -----------------------*/
        else if(key == '*')
        {
            return PASS_FAIL;
        }
        else if(key == 0xFF)
            return PASS_FAIL;
    }
//    return PASS_FAIL;
}


/*=============================================================
 * menu_vote(user)
 * Option 3 — Voting
 * Shows 5 party symbols from CGRAM.
 * One vote per user — flag stored in EEPROM.
 * '*' or timeout exits.
 *=============================================================*/
s8 menu_vote(s8 user)
{
    u8   key;
	u16  cnt_addr = 0;
    u8   sym      = 0;
    s8  *party_name = (s8*)"";

    if(eep_get_vote(user) == VOTED)
    {
        clr();
        CmdLCD(L1); StrLCD((s8*)"Already Voted!");
        CmdLCD(L2); StrLCD((s8*)"One vote per user");
        CmdLCD(L3); StrLCD((s8*)"Thank You.");
        CmdLCD(L4); StrLCD((s8*)"*=Exit ");
        set_wait_valid_keys((s8*)"*");
        timed_wait_key(AUTO_TIMEOUT_S);
        return PASS_FAIL;
    }

    vote_load_symbols();

    clr();
    CmdLCD(L1); StrLCD((s8*)"--- VOTE MENU --");
    CmdLCD(L2);
    StrLCD((s8*)"1-A"); CharLCD(SYM_PARTY_A);
    StrLCD((s8*)"  2-B"); CharLCD(SYM_PARTY_B);
    CmdLCD(L3);
    StrLCD((s8*)"3-C"); CharLCD(SYM_PARTY_C);
    StrLCD((s8*)"  4-D"); CharLCD(SYM_PARTY_D);
    CmdLCD(L4);
    StrLCD((s8*)"5-NOTA"); CharLCD(SYM_NOTA);
    StrLCD((s8*)" *=Exit");

    set_wait_valid_keys((s8*)"12345*");
    key = timed_wait_key(AUTO_TIMEOUT_S);

    if     (key=='1'){ cnt_addr=EEP_CNT_A;    sym=SYM_PARTY_A; party_name=(s8*)"Party A"; }
    else if(key=='2'){ cnt_addr=EEP_CNT_B;    sym=SYM_PARTY_B; party_name=(s8*)"Party B"; }
    else if(key=='3'){ cnt_addr=EEP_CNT_C;    sym=SYM_PARTY_C; party_name=(s8*)"Party C"; }
    else if(key=='4'){ cnt_addr=EEP_CNT_D;    sym=SYM_PARTY_D; party_name=(s8*)"Party D"; }
    else if(key=='5'){ cnt_addr=EEP_CNT_NOTA; sym=SYM_NOTA;    party_name=(s8*)"NOTA";    }
    else if(key=='*'){ return PASS_FAIL; }   /* '*' or timeout = exit */
	else if(key==0xFF){ return PASS_FAIL; }

    /* Record vote */
    eep_inc_party(cnt_addr);
    eep_set_vote(user, VOTED);

    clr();
    CmdLCD(L1); StrLCD((s8*)"Vote Recorded!");
    CmdLCD(L2); StrLCD(party_name); StrLCD((s8*)" "); CharLCD(sym);
    CmdLCD(L3); StrLCD((s8*)"Thank You.");
    CmdLCD(L4); StrLCD((s8*)"*=Exit ");
    set_wait_valid_keys((s8*)"*");
    timed_wait_key(AUTO_TIMEOUT_S);
    return PASS_FAIL;
}


/*=============================================================
 * menu_dl(user)
 * Option 4 — Driving Licence
 *
 * Reads expiry DD/MM/YYYY from EEPROM.
 * Reads real DD/MM/YYYY + HH:MM from RTC.
 *
 * VALID check (full date comparison):
 *   Expired if: rtc_year  > exp_year
 *            OR rtc_year == exp_year AND rtc_month  > exp_month
 *            OR rtc_year == exp_year AND rtc_month == exp_month
 *                                    AND rtc_date   > exp_day
 *
 * LCD:
 *   Line1: user name
 *   Line2: DL number
 *   Line3: Class + Expiry DD/MM/YYYY
 *   Line4: VALID   DD/MM/YYYY HH:MM   (if not expired)
 *      or  EXPIRED DD/MM/YYYY HH:MM   (if expired)
 *
 * '*' or 10s timeout exits.
 *=============================================================*/
static void dl_show_rtc_then_wait(s8 user)
{
    s32 h, m, sec, dd, mm, yy, dow;
    u8 key;
    u8 tick;
    static const s8 *days[7] = {
        (s8*)"SUN", (s8*)"MON", (s8*)"TUE", (s8*)"WED",
        (s8*)"THU", (s8*)"FRI", (s8*)"SAT"
    };

    /* Show a live RTC screen for five seconds before the DL details. */
    clr();
    for(tick = 0; tick < 5; tick++)
    {
        /* Read RTC again every second so the displayed time really runs. */
        GetRTCTimeInfo(&h, &m, &sec);
        GetRTCDateInfo(&dd, &mm, &yy);
        GetRTCDay(&dow);

        CmdLCD(L1); StrLCD((s8*)"TIME ");
        CharLCD((u8)('0' + h/10)); CharLCD((u8)('0' + h%10));
        CharLCD(':');
        CharLCD((u8)('0' + m/10)); CharLCD((u8)('0' + m%10));
        CharLCD(':');
        CharLCD((u8)('0' + sec/10)); CharLCD((u8)('0' + sec%10));

        CmdLCD(L2); StrLCD((s8*)"DATE ");
        CharLCD((u8)('0' + dd/10)); CharLCD((u8)('0' + dd%10));
        CharLCD('/');
        CharLCD((u8)('0' + mm/10)); CharLCD((u8)('0' + mm%10));
        CharLCD('/');
        U32LCD((u32)yy);

        CmdLCD(L3); StrLCD((s8*)"DAY  ");
        if(dow < 0 || dow > 6) dow = 0;
        StrLCD((s8*)days[dow]);
        CmdLCD(L4); StrLCD((s8*)"* = Exit");

        /* Wait one second; invalid keys are consumed and ignored. */
        for(m = 0; m < 10; m++)
        {
            if(colscan() == 0)
            {
                key = keyscan();
                while(colscan() == 0);
                if(key == '*')
                    return;
            }
            delay_ms(100);
        }
    }

    /* Display DL details after the live RTC preview. */
    while(1)
    {
        s8 *dl_name, *dl_num, *dl_class;
        u8 exp_dd, exp_mm;
        u16 exp_yyyy;
        u8 expired;

        /* Read current RTC again for an accurate validity decision. */
        GetRTCDateInfo(&dd, &mm, &yy);
        eep_get_dl_expiry(user, &exp_dd, &exp_mm, &exp_yyyy);

        if((u32)yy > (u32)exp_yyyy ||
           ((u32)yy == (u32)exp_yyyy && (u32)mm > (u32)exp_mm) ||
           ((u32)yy == (u32)exp_yyyy && (u32)mm == (u32)exp_mm &&
            (u32)dd > (u32)exp_dd))
            expired = 1;
        else
            expired = 0;

        if(user == USER1)
        { dl_name=(s8*)DL_NAME_U1; dl_num=(s8*)DL_NUM_U1; dl_class=(s8*)DL_CLASS_U1; }
        else if(user == USER2)
        { dl_name=(s8*)DL_NAME_U2; dl_num=(s8*)DL_NUM_U2; dl_class=(s8*)DL_CLASS_U2; }
        else
        { dl_name=(s8*)DL_NAME_U3; dl_num=(s8*)DL_NUM_U3; dl_class=(s8*)DL_CLASS_U3; }

        clr();
        CmdLCD(L1); StrLCD(dl_name);
        CmdLCD(L2); StrLCD(dl_num);
        CmdLCD(L3); StrLCD((s8*)"C:"); StrLCD(dl_class);
        StrLCD((s8*)" E:");
        CharLCD((u8)('0' + exp_dd/10)); CharLCD((u8)('0' + exp_dd%10));
        CharLCD('/');
        CharLCD((u8)('0' + exp_mm/10)); CharLCD((u8)('0' + exp_mm%10));
        CharLCD('/'); U32LCD((u32)exp_yyyy);
        CmdLCD(L4);
        if(expired) StrLCD((s8*)"EXPIRED  *=Exit");
        else        StrLCD((s8*)"VALID    *=Exit");

        /* Only '*' is meaningful here; all other keys are ignored. */
        set_wait_valid_keys((s8*)"*");
        key = timed_wait_key(AUTO_TIMEOUT_S);
        if(key == '*') return;
        if(key == 0xFF) return;
    }
}

/*=============================================================
 * menu_dl(user)
 *
 * Exactly two valid actions:
 *   1 = show current RTC time/date/day, then DL interface
 *   * = exit
 *
 * Any other keypad key is ignored. If there is no valid action
 * for AUTO_TIMEOUT_S, the function returns automatically.
 *=============================================================*/
s8 menu_dl(s8 user)
{
    u8 key;

    while(1)
    {
        clr();
        CmdLCD(L1); StrLCD((s8*)"-- DL MENU --");
        CmdLCD(L2); StrLCD((s8*)"1-View DL");
        CmdLCD(L3); StrLCD((s8*)"*-Exit");

        set_wait_valid_keys((s8*)"1*");
        key = timed_wait_key(AUTO_TIMEOUT_S);

        if(key == '1')
        {
            dl_show_rtc_then_wait(user);
            return PASS_FAIL;
        }
        if(key == '*')
            return PASS_FAIL;
        if(key == 0xFF)
            return PASS_FAIL;
        /* Ignore every other key and continue waiting. */
    }
//    return PASS_FAIL;
}


/*=============================================================
 * show_menu(user)
 * Main service menu for a valid user.
 * '*' exits to idle. Options 1-5 call service functions.
 * Auto-timeout = 10s — returns to idle.
 *=============================================================*/
s8 show_menu(s8 user)
{
    u8 key;

    while(1)
    {
        clr();
        CmdLCD(L1); StrLCD((s8*)"1-PAN   2-ATM");
        CmdLCD(L2); StrLCD((s8*)"3-Vote  4-DriveLic");
        CmdLCD(L3); StrLCD((s8*)"5-ChgPass  ");
        CmdLCD(L4); StrLCD((s8*)"*=Exit ");

        set_wait_valid_keys((s8*)"12345*");
        key = timed_wait_key(15);

        if(key == MENU_PAN)
        {
            if(menu_pan(user) == PASS_LOCKED) 
				return PASS_LOCKED;
        }
        else if(key == MENU_ATM)
        {
            if(menu_atm(user) == PASS_LOCKED) 
				return PASS_LOCKED;
        }
        else if(key == MENU_VOTE)
            menu_vote(user);
        else if(key == MENU_DL)
            menu_dl(user);
        else if(key == MENU_CHGPS)
            menu_change_password();
        else if(key == '*')
            return PASS_FAIL;
        else if(key == 0xFF)
            return PASS_FAIL;
       
    }
//    return PASS_FAIL;
}
