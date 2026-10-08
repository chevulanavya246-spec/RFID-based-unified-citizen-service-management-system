//spi_defines.h
//defines for SPI0 Pin Function Select

/*
 * LPC2148 SPI0 pin mapping (PINSEL0):
 *   P0.4 = SCK0  → PINSEL0 bits 9:8  = 01 → value 0x00000100
 *   P0.5 = MOSI0 → PINSEL0 bits 11:10= 01 → value 0x00000400
 *   P0.6 = MISO0 → PINSEL0 bits 13:12= 01 → value 0x00001000
 *   P0.7 = CS    → GPIO output (manual control)
 *
 * BUG FIXED: MOSI0 and MISO0 were SWAPPED in original file.
 *   Old MOSI0 = 0x00001000 (P0.6) ← WRONG, that is MISO pin
 *   Old MISO0 = 0x00000400 (P0.5) ← WRONG, that is MOSI pin
 *   This caused SPI data lines to be crossed → EEPROM read/write
 *   returned garbage → card IDs never matched → INVALID CARD.
 */
#define SCK0     0x00000100  /* P0.4  bits 9:8  = 01  */
#define MOSI0    0x00000400  /* P0.5  bits 11:10= 01  FIX: was 0x00001000 */
#define MISO0    0x00001000  /* P0.6  bits 13:12= 01  FIX: was 0x00000400 */
#define CS       7           /* P0.7  GPIO manual CS  */

// SxSPCR sfr bit defines
#define CPOL_BIT   3
#define CPHA_BIT   4
#define MSTR_BIT   5   // SPI0 as Master
#define LSBF_BIT   6   // default MSB first, if set LSB first

// SxSPSR sfr bit defines
#define SPIF_BIT   7   // Data Transfer Completion Flag
