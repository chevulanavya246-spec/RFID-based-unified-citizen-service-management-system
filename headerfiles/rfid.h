/*=============================================================
 * rfid.h
 *=============================================================*/
#ifndef RFID_H
#define RFID_H

#include "rfid_defines.h"
#include "types.h"

/* Set LED + buzzer pins as output */
void rfid_gpio_init(void);

/* Write all 4 card IDs into EEPROM — call ONCE on first flash */
void rfid_store_cards(void);

/* Read 10-byte packet from UART0, extract 8-byte card ID */
void rfid_read_card(u8 *buf);

/* Compare buf[] against EEPROM cards, return user index */
s8   rfid_get_user(u8 *buf);

/* Get username string for a user index */
void rfid_get_username(s8 user_index, u8 *name_buf);

/* Green LED blink = valid */
void rfid_valid_indication(void);

/* Red LED + buzzer = invalid */
void rfid_invalid_indication(void);

#endif
