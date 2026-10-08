/*=============================================================
 * rfid_defines.h
 *=============================================================*/
#ifndef RFID_DEFINES_H
#define RFID_DEFINES_H


/*--- RFID packet -------------------------------------------*/
#define RFID_CARD_LEN    8       /* 8 ASCII bytes per card    */
#define RFID_START      0x02    /* STX                        */
#define RFID_END        0x03    /* ETX                        */

/*--- Card IDs (replace with your real card numbers) --------*/
#define CARD_USER1      "12527137"
#define CARD_USER2      "12524422"
#define CARD_USER3      "12638593"
#define CARD_OFFICER    "12547508"

/*--- User index return values ------------------------------*/
#define USER1            0
#define USER2            1
#define USER3            2
#define OFFICER          3
#define INVALID_CARD    -1

/*--- User names --------------------------------------------*/
#define NAME_USER1      "Mani teja"
#define NAME_USER2      "Santhosh Pawar"
#define NAME_USER3      "Guru Prasad"

/*--- EEPROM card ID addresses (8 bytes each) ---------------*
 *  0x0010 - 0x0017 : User1 card ID
 *  0x0018 - 0x001F : User2 card ID
 *  0x0020 - 0x0027 : User3 card ID
 *  0x0028 - 0x002F : Officer card ID
 *----------------------------------------------------------*/
#define EEP_CARD_USER1    0x0010
#define EEP_CARD_USER2    0x0018
#define EEP_CARD_USER3    0x0020
#define EEP_CARD_OFFICER  0x0028

#define EEP_CARD_VERSION 0x0004
#define CARD_VERSION     0X02

/*--- LED / Buzzer pins -------------------------------------*/
#define GREEN_LED        (1<<19)
#define RED_LED          (1<<20)
#define BUZZER           (1<<21)

#endif
