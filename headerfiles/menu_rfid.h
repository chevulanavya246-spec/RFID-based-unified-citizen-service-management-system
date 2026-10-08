/*=============================================================
 * menu_rfid.h
 * RFID-Based Unified Citizen Service Management System
 *=============================================================*/
#ifndef MENU_RFID_H
#define MENU_RFID_H

#include "types.h"
#include "rfid_defines.h"
#include "menu_rfid_defines.h"

/* Password/menu status values used to return a 3-attempt lock to main(). */
#define PASS_OK       1
#define PASS_FAIL     0
#define PASS_LOCKED  -2

/*--- Password (EEPROM backed, survives power off) -----------*/
void pass_read_eeprom(u8 *buf);     /* read 4 digits          */
void pass_write_eeprom(u8 *buf);    /* write 4 digits         */

/*--- First flash EEPROM init --------------------------------*/
void menu_init_eeprom(void);

/*--- Officer menu (1=reset votes, 2=set DL expiry) ----------*/
void officer_menu(void);

/*--- Main user service menu ---------------------------------*/
s8 show_menu(s8 user);

/*--- Service functions (called from show_menu) --------------*/
s8 menu_pan(s8 user);             /* 1 - PAN Card          */
s8 menu_atm(s8 user);             /* 2 - ATM               */
s8 menu_vote(s8 user);            /* 3 - Voting            */
s8 menu_dl(s8 user);              /* 4 - Driving Licence   */
s8 menu_change_password(void);    /* 5 - Change Password   */

/*--- EEPROM helpers -----------------------------------------*/
u32  eep_get_balance(s8 user);
void eep_set_balance(s8 user, u32 amount);
u8   eep_get_vote(s8 user);
void eep_set_vote(s8 user, u8 status);
void vote_reset_counts(void);

#endif /* MENU_RFID_H */
