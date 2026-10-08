/*=============================================================
 * menu_rfid_defines.h
 * All user data, EEPROM addresses, ATM rules, menu keys
 *=============================================================*/
#ifndef MENU_DEFINES_H
#define MENU_DEFINES_H

/*-------------------------------------------------------------
 * Password (default — auto-written to EEPROM on first power-on)
 *-------------------------------------------------------------*/
#define PASS_LEN             4
#define DEFAULT_PASSWORD     "1111"

/* EEPROM test is it initialised or not */
#define EEP_TEST_ADDR 0x01f0
#define EEP_TEST_VALUE 0X5A

/* Project-level EEPROM initialization marker.
 * These bytes are independent of all user data.
 * A new/unknown lab EEPROM is initialized when this signature/version
 * is not present. After that, normal updates are preserved. */
#define EEP_PROJECT_MAGIC_0  0x00
#define EEP_PROJECT_MAGIC_1  0x02
#define EEP_PROJECT_MAGIC_2  0x03
#define EEP_PROJECT_MAGIC_3  0x05
#define EEP_PROJECT_VERSION  0x03

/* Saved RTC values: HH, MM, SS, DD, MM, DOW, YY-high, YY-low */
#define EEP_RTC_HOUR         0x0068
#define EEP_RTC_MIN          0x0069
#define EEP_RTC_SEC          0x006A
#define EEP_RTC_DAY          0x006B
#define EEP_RTC_MONTH        0x006C
#define EEP_RTC_DOW          0x006D
#define EEP_RTC_YEAR_H       0x006E
#define EEP_RTC_YEAR_L       0x006F

/*-------------------------------------------------------------
 * Auto-timeout — every screen exits after 10s of no key press
 * '*' key also exits any screen immediately
 *-------------------------------------------------------------*/
#define AUTO_TIMEOUT_S       10

/*-------------------------------------------------------------
 * PAN Card data  (edit names/DOB/PAN to match real users)
 *-------------------------------------------------------------*/
#define PAN_NAME_U1          "Mani Teja"
#define PAN_DOB_U1           "01-01-1990"
#define PAN_NUM_U1           "ABCDE1234F"

#define PAN_NAME_U2          "Santhosh Pawar"
#define PAN_DOB_U2           "15-06-1995"
#define PAN_NUM_U2           "PQRST5678G"

#define PAN_NAME_U3          "Guru Prasad"
#define PAN_DOB_U3           "22-03-1988"
#define PAN_NUM_U3           "XYZAB9012H"

/*-------------------------------------------------------------
 * Driving Licence data
 * Expiry full date (DD/MM/YYYY) stored in EEPROM.
 * Officer card sets it to RTC current date + 20 years.
 * DL_EXPIRY_U1/U2/U3 = initial year written on first flash.
 * DL_EXPIRY_DD / DL_EXPIRY_MM = initial day and month (01/01).
 *-------------------------------------------------------------*/
#define DL_NAME_U1           "Mani Teja"
#define DL_NUM_U1            "DL-01-2019-001"
#define DL_CLASS_U1          "LMV"
#define DL_EXPIRY_U1         2026

#define DL_NAME_U2           "Santhosh Pawar"
#define DL_NUM_U2            "DL-02-2020-045"
#define DL_CLASS_U2          "LMV"
#define DL_EXPIRY_U2         2025

#define DL_NAME_U3           "Guru Prasad"
#define DL_NUM_U3            "DL-03-2018-099"
#define DL_CLASS_U3          "HMV"
#define DL_EXPIRY_U3         2027

#define DL_EXPIRY_DD         1       /* initial day   = 01    */
#define DL_EXPIRY_MM         1       /* initial month = Jan   */

/*-------------------------------------------------------------
 * EEPROM Memory Map
 *
 * 0x0000,0x0002,0x0003,0x0005 : project EEPROM signature/version
 * 0x0010-0x002F   : Card IDs (8 bytes each x 4 cards)
 *
 * Balance (4 bytes per user, u32, MSB first — supports lakhs):
 *   User1 : 0x0030 0x0031 0x0032 0x0033
 *   User2 : 0x0034 0x0035 0x0036 0x0037
 *   User3 : 0x0038 0x0039 0x003A 0x003B
 *
 * Vote flag (1 byte per user: 0x00=not voted, 0x01=voted):
 *   User1 : 0x003C
 *   User2 : 0x003D
 *   User3 : 0x003E
 *
 * Vote party counts (1 byte each):
 *   A:0x0040  B:0x0041  C:0x0042  D:0x0043  NOTA:0x0044
 *
 * DL expiry full date (DD, MM, YH, YL per user):
 *   User1 : 0x0050(DD) 0x0051(MM) 0x0052(YH) 0x0053(YL)
 *   User2 : 0x0054(DD) 0x0055(MM) 0x0056(YH) 0x0057(YL)
 *   User3 : 0x0058(DD) 0x0059(MM) 0x005A(YH) 0x005B(YL)
 *
 * Password (4 bytes, one ASCII digit each):
 *   0x0060  0x0061  0x0062  0x0063
 *
 * RTC saved state:
 *   0x0068-0x006F
 *-------------------------------------------------------------*/

/* Balance base address per user (4 bytes from here) */
#define EEP_BAL_H_U1         0x0030
#define EEP_BAL_H_U2         0x0034
#define EEP_BAL_H_U3         0x0038

/* Vote flags */
#define EEP_VOTE_U1          0x003C
#define EEP_VOTE_U2          0x003D
#define EEP_VOTE_U3          0x003E

/* Vote state values */
#define NOT_VOTED            0x00
#define VOTED                0x01

/* Initial balance on first flash */
#define INIT_BALANCE         5000

/* Party vote count addresses */
#define EEP_CNT_A            0x0040
#define EEP_CNT_B            0x0041
#define EEP_CNT_C            0x0042
#define EEP_CNT_D            0x0043
#define EEP_CNT_NOTA         0x0044
#define TOTAL_PARTIES        5

/* DL expiry full date addresses */
#define EEP_DL_DD_U1         0x0050
#define EEP_DL_MM_U1         0x0051
#define EEP_DL_YH_U1         0x0052
#define EEP_DL_YL_U1         0x0053

#define EEP_DL_DD_U2         0x0054
#define EEP_DL_MM_U2         0x0055
#define EEP_DL_YH_U2         0x0056
#define EEP_DL_YL_U2         0x0057

#define EEP_DL_DD_U3         0x0058
#define EEP_DL_MM_U3         0x0059
#define EEP_DL_YH_U3         0x005A
#define EEP_DL_YL_U3         0x005B

/* Password addresses */
#define EEP_PASS_0           0x0060
#define EEP_PASS_1           0x0061
#define EEP_PASS_2           0x0062
#define EEP_PASS_3           0x0063

/* CGRAM symbol slots for voting */
#define SYM_PARTY_A          0
#define SYM_PARTY_B          1
#define SYM_PARTY_C          2
#define SYM_PARTY_D          3
#define SYM_NOTA             4

/*-------------------------------------------------------------
 * ATM Rules
 *  Notes      : Rs.100 and Rs.500 only (amount % 100 == 0)
 *  Withdraw   : min Rs.100, max Rs.45000 per transaction
 *  Min balance: Rs.500 must remain after withdraw
 *  Deposit    : min Rs.100, max Rs.45000 per transaction
 *  Max balance: NO limit (u32 — can be lakhs)
 *-------------------------------------------------------------*/
#define NOTE_MULTIPLE        100
#define MIN_WITHDRAW         100
#define MAX_WITHDRAW         45000
#define MIN_BALANCE          500
#define MIN_DEPOSIT          100
#define MAX_DEPOSIT          45000

/*-------------------------------------------------------------
 * Menu option keys (keypad ASCII)
 *-------------------------------------------------------------*/
#define MENU_PAN             '1'
#define MENU_ATM             '2'
#define MENU_VOTE            '3'
#define MENU_DL              '4'
#define MENU_CHGPS           '5'

#define ATM_BALANCE          '1'
#define ATM_WITHDRAW         '2'
#define ATM_DEPOSIT          '3'

#endif /* MENU_DEFINES_H */
