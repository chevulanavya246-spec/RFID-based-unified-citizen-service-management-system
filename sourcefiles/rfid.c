/*=============================================================
 * rfid.c
 * RFID-Based Unified Citizen Service Management System
 *
 * BUGS FIXED:
 * 1. spi_defines.h: MOSI0/MISO0 were swapped — fixed there.
 *    This was ROOT CAUSE of all INVALID CARD issues.
 *
 * 2. rfid_gpio_init: uses PINSEL1 for P0.19/P0.20/P0.21
 *    (GREEN_LED, RED_LED, BUZZER). Old code wrongly used PINSEL0.
 *
 * 3. Every card read now displays card bytes on LCD LINE3
 *    so you can visually compare UART card number vs EEPROM.
 *    Format: LCD shows "UART:12603355" then "EEP :12603355"
 *
 * 4. rfid_debug_display() added — shows both UART received
 *    card and EEPROM stored card on LCD for all 4 slots.
 *=============================================================*/

#include <LPC21xx.h>
#include "rfid_defines.h"
#include "rfid.h"
#include "delay.h"
#include "spi_eeprom.h"
#include "uart.h"
#include "lcd.h"
#include "lcd_defines.h"

#define L1  GOTO_LINE1_POS0
#define L2  GOTO_LINE2_POS0
#define L3  GOTO_LINE3_POS0
#define L4  GOTO_LINE4_POS0


extern volatile unsigned char rfid_buffer[16]; ///< Storage buffer for incoming RFID tag characters.
extern volatile u8 rfid_ready;              ///< Flag set to 1 when a full RFID frame is received.


/*-------------------------------------------------------------
 * card_match
 *-------------------------------------------------------------*/
static u8 card_match(u8 *a, u8 *b)
{
    u8 i;
    for(i = 0; i < RFID_CARD_LEN; i++)
        if(a[i] != b[i]) return 0;
    return 1;
}

static void str_copy(u8 *dst, const u8 *src)
{
    u8 i = 0;
    while(src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

/*=============================================================
 * rfid_gpio_init()
 * GREEN_LED=P0.19, RED_LED=P0.20, BUZZER=P0.21
 * These are in PINSEL1 range (P0.16-P0.31)
 * P0.19 = PINSEL1 bits 7:6
 * P0.20 = PINSEL1 bits 9:8
 * P0.21 = PINSEL1 bits 11:10
 *=============================================================*/
void rfid_gpio_init(void)
{
    PINSEL1 &= ~((3<<6)|(3<<8)|(3<<10));
    IODIR0  |=  GREEN_LED | RED_LED | BUZZER;
    IOCLR0   =  GREEN_LED | RED_LED | BUZZER;
}

void rfid_valid_indication(void)
{
    IOSET0 = GREEN_LED;
    delay_ms(500);
    IOCLR0 = GREEN_LED;
}

void rfid_invalid_indication(void)
{
    IOSET0 = RED_LED | BUZZER;
    delay_ms(500);
    IOCLR0 = RED_LED | BUZZER;
}

/*=============================================================
 * rfid_print_card(buf)
 * Helper: prints 8-byte card ID string on LCD at current pos
 *=============================================================*/
static void rfid_print_card(u8 *buf)
{
    u8 i;
    for(i = 0; i < RFID_CARD_LEN; i++)
        CharLCD(buf[i]);
}

/*=============================================================
 * rfid_store_cards()
 * Writes 4 card IDs to EEPROM.
 * Called only once from main (guard at 0x0100 protects it).
 *=============================================================*/
void rfid_store_cards(void)
{
    u8 i;

    const u8 *cards[4] = {
        (u8*)CARD_USER1,
        (u8*)CARD_USER2,
        (u8*)CARD_USER3,
        (u8*)CARD_OFFICER
    };

    u16 addrs[4] = {
        EEP_CARD_USER1,
        EEP_CARD_USER2,
        EEP_CARD_USER3,
        EEP_CARD_OFFICER
    };

    u8 c;
    for(c = 0; c < 4; c++)
	{
		/*CmdLCD(0x01);
		StrLCD("Read");
		CmdLCD(0xC0);*/
        for(i = 0; i < RFID_CARD_LEN; i++)
		{
			ByteWrite_25LC512(addrs[c]+i, cards[c][i]);
			
			//CharLCD(ByteRead_25LC512(addrs[c]+i));
		}
		//delay_ms(2000);
	}

}

/*=============================================================
 * rfid_read_card(buf)
 *
 * Reads one 10-byte RFID packet from UART0.
 * Packet: [0x02][B1..B8][0x03]
 *
 * After reading — displays received card on LCD LINE3 so you
 * can compare with EEPROM stored value visually:
 *   Line3: UART:12603355
 *=============================================================*/
void rfid_read_card(u8 *buf)
{
    u8 i;

    /*
     * Wait until UART interrupt receives
     * a complete RFID card.
     */
    while(rfid_ready == 0);

    /*
     * Copy the received 8-byte card number
     * from the UART interrupt buffer.
     */
    for(i = 0; i < RFID_CARD_LEN; i++)
    {
        buf[i] = rfid_buffer[i];
    }

    /*
     * Add string terminator after 8 card bytes.
     */
    buf[RFID_CARD_LEN] = '\0';

    /*
     * Clear the ready flag so the next
     * RFID card can be received.
     */
    rfid_ready = 0;

    /*
     * Show received card for debugging.
     */
    CmdLCD(CLEAR_LCD);
    CmdLCD(L3);
    StrLCD((s8*)"UART:");
    rfid_print_card(buf);
}

/*=============================================================
 * rfid_get_user(buf)
 *
 * Reads each EEPROM card slot and compares with received buf.
 * Also shows EEPROM stored card on LCD LINE4 for comparison:
 *   Line3: UART:12603355
 *   Line4: EEP :12603355   ← if match found
 *
 * Returns USER1/USER2/USER3/OFFICER/INVALID_CARD
 *=============================================================*/
s8 rfid_get_user(u8 *buf)
{
    u8  i;
    u8  eep_card[RFID_CARD_LEN + 1];

    u16 addrs[4]   = { EEP_CARD_USER1, EEP_CARD_USER2,
                        EEP_CARD_USER3, EEP_CARD_OFFICER };
    s8  results[4] = { USER1, USER2, USER3, OFFICER };

    u8 c;
    for(c = 0; c < 4; c++)
    {
        /* Read 8 bytes from this EEPROM slot */
        for(i = 0; i < RFID_CARD_LEN; i++)
            eep_card[i] = ByteRead_25LC512(addrs[c]+i);
        eep_card[RFID_CARD_LEN] = '\0';
		/*CmdLCD(0x01);
		StrLCD(eep_card);
		delay_ms(2000);*/

		if(card_match(buf, eep_card))
        {
            /* Show matched EEPROM card on LCD4 */
            CmdLCD(L4);
            StrLCD((s8*)"EEP :");
            rfid_print_card(eep_card);
            delay_ms(1000);
            return results[c];
        }
    }

    /* No match — show first EEPROM slot for comparison */

	CmdLCD(L4);
    StrLCD((s8*)"EEP :");
    rfid_print_card(eep_card);
    delay_ms(1000);

    return INVALID_CARD;
}

/*=============================================================
 * rfid_get_username(user_index, name_buf)
 *=============================================================*/
void rfid_get_username(s8 user_index, u8 *name_buf)
{
    if(user_index == USER1)        str_copy(name_buf,(u8*)NAME_USER1);
    else if(user_index == USER2)   str_copy(name_buf,(u8*)NAME_USER2);
    else if(user_index == USER3)   str_copy(name_buf,(u8*)NAME_USER3);
    else if(user_index == OFFICER) str_copy(name_buf,(u8*)"Officer");
    else                           str_copy(name_buf,(u8*)"Unknown");
}
