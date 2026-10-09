RFID-Based Unified Citizen Service Management System

📌 Project Overview

The RFID-Based Unified Citizen Service Management System is an embedded system project developed using the LPC2148 ARM7 microcontroller.

The system uses an RFID card as a unique citizen identification mechanism. When a citizen scans an RFID card, the RFID reader sends the card information to the LPC2148 through UART. The microcontroller verifies the received card ID with the card IDs stored in SPI EEPROM.

After successful authentication, the citizen can access different services through a 16×2/20×4 LCD interface and keypad-based menu system.

The project integrates RFID, UART, LCD, keypad, SPI EEPROM and RTC peripherals into a single embedded platform.

---

🎯 Objectives

- Provide a unified interface for multiple citizen services.
- Identify users using RFID cards.
- Authenticate RFID cards before providing access.
- Store user information and service data in EEPROM.
- Provide password-protected services.
- Provide ATM-style balance operations.
- Provide a voting facility with one-vote-per-user logic.
- Check Driving Licence expiry using RTC.
- Provide an officer card for administrative operations.
- Store important information permanently in SPI EEPROM.
- Provide valid/invalid card indications using LED and buzzer.

---

🏗️ System Architecture

                ┌─────────────────┐
                │    RFID Card    │
                └────────┬────────┘
                         │
                         ▼
                ┌─────────────────┐
                │   RFID Reader   │
                └────────┬────────┘
                         │ UART
                         ▼
              ┌──────────────────────┐
              │    LPC2148 ARM7      │
              │    Microcontroller   │
              └───────┬──────┬───────┘
                      │      │
          ┌───────────┘      └────────────┐
          ▼                               ▼
   ┌──────────────┐                ┌──────────────┐
   │     LCD      │                │    Keypad    │
   │   Display    │                │   4 × 4      │
   └──────────────┘                └──────────────┘
          │                               │
          └───────────┬───────────────────┘
                      ▼
              ┌─────────────────┐
              │   Service Menu  │
              └────────┬────────┘
                       │
       ┌───────────────┼────────────────┐
       ▼               ▼                ▼
   PAN CARD          ATM             VOTING
       │               │                │
       └───────────────┼────────────────┘
                       ▼
              DRIVING LICENCE
                       │
                       ▼
              ┌─────────────────┐
              │  SPI EEPROM     │
              │   AT25LC512     │
              └────────┬────────┘
                       │
                       ▼
              ┌─────────────────┐
              │      RTC        │
              │ Date & Time     │
              └─────────────────┘

---

🔄 Working Principle

Step 1 — System Initialization

When the system is powered ON, the LPC2148 initializes:

- UART0
- SPI0
- LCD
- Keypad
- RFID GPIO
- RTC

The system displays an RFID citizen service splash screen.

The project also performs an EEPROM connectivity test. If the EEPROM is not detected, the LCD displays an EEPROM error and the system stops.

Step 2 — EEPROM Initialization

During the first project setup, the system checks a dedicated RFID project MAGIC/VERSION signature in EEPROM.

If the signature is not available, the system writes the initial project data, including:

- RFID card IDs
- User balances
- Voting status
- Voting counts
- Driving Licence expiry information
- Password
- RTC information

The firmware then stores a project initialization marker so that the default data is not unnecessarily rewritten on every power-up.

Step 3 — RFID Card Scanning

The system waits for an RFID card.

The RFID reader sends a 10-byte packet through UART:

[0x02] [8-byte Card ID] [0x03]

The actual user/card ID used for comparison is 8 bytes. The received card data is copied from the UART receive buffer and compared against the card IDs stored in EEPROM.

Step 4 — Card Verification

The received RFID card is compared with four stored card entries:

USER1
USER2
USER3
OFFICER

If a matching card is found, the corresponding user is identified.

If no card matches, the system treats it as an invalid card.

Step 5 — Valid / Invalid Indication

For a valid card:

GREEN LED → ON

For an invalid card:

RED LED + BUZZER → ON

The LCD also displays the corresponding access status.

---

👤 User Services

After a valid citizen card is authenticated, the system provides a service menu.

The main services implemented are:

1. PAN CARD
2. ATM
3. VOTING
4. DRIVING LICENCE
5. EXIT

---

🪪 1. PAN Card Service

The PAN Card service is password protected.

After successful password authentication, the system displays:

- Citizen Name
- Date of Birth
- PAN Number

The PAN information is stored in the project configuration and selected according to the authenticated user.

---

💳 2. ATM Service

The ATM module provides:

Balance Enquiry

The current balance is read from EEPROM and displayed on the LCD.

Withdrawal

The system validates:

- Minimum withdrawal amount
- Maximum transaction amount
- Valid note multiples
- Available balance
- Minimum remaining balance

The current implementation uses:

Minimum withdrawal : ₹100
Maximum transaction: ₹45,000
Minimum balance    : ₹500

The balance is written back to EEPROM after a successful transaction.

Deposit

The user can deposit money after authentication.

The system validates the transaction amount and stores the updated balance in EEPROM.

---

🗳️ 3. Voting Service

The voting module provides a one-vote-per-user mechanism.

The LCD displays five choices:

1 - Party A
2 - Party B
3 - Party C
4 - Party D
5 - NOTA

After a vote is recorded:

Party Count → Incremented
User Vote Flag → VOTED

If the same user tries to vote again, the system displays:

Already Voted!
One vote per user

The voting status and vote counts are stored in EEPROM.

---

🚗 4. Driving Licence Service

The Driving Licence module uses:

EEPROM → Stored expiry date
RTC    → Current date/time

The stored Driving Licence expiry date is compared with the current RTC date.

The system can therefore determine whether the licence is:

VALID

or

EXPIRED

The project also provides an officer function for updating an expired licence expiry date.

---

👮 Officer Card

The project includes a separate Officer RFID Card.

When the officer card is scanned, the system opens:

-- OFFICER MENU --

1 - Reset Votes
2 - Set DL Expiry
* - Exit

Reset Votes

This operation resets:

- Party A count
- Party B count
- Party C count
- Party D count
- NOTA count
- User voting flags

Set Driving Licence Expiry

The officer can select a user through the keypad.

The system checks the existing expiry date against the RTC date. If the licence has expired, the system updates the expiry date using the current date and a 20-year period.

---

🔐 Password Protection

The project uses a password stored in EEPROM.

Password verification provides three attempts.

Correct Password
       ↓
Access Granted

Wrong Password
       ↓
Attempts Remaining

3 Wrong Attempts
       ↓
ACCESS LOCKED

The user can also change the password by:

Current Password
       ↓
New Password
       ↓
Confirm Password
       ↓
Password Saved to EEPROM

The password is therefore retained even after power is removed.

---

💾 SPI EEPROM

The project uses AT25LC512 SPI EEPROM for non-volatile data storage.

EEPROM is used to store project information such as:

- RFID card IDs
- Password
- User balances
- Voting flags
- Party vote counts
- Driving Licence expiry dates
- RTC saved values
- Project initialization information

The source code includes dedicated SPI and SPI EEPROM modules.

---

🕐 RTC

The RTC module provides:

- Current time
- Current date
- Day information

The project saves RTC information to EEPROM and restores it during initialization when the project has already been initialized.

RTC is particularly important for the Driving Licence expiry verification.

---

📺 LCD

The LCD is used to display:

- System startup messages
- RFID scanning status
- Valid/invalid card status
- User information
- Service menus
- Password prompts
- ATM transactions
- Voting options
- Driving Licence information
- Officer menu
- Error messages

The repository contains dedicated LCD source and header files.

---

⌨️ Keypad

The keypad provides user input for:

- Service selection
- Password entry
- ATM amount entry
- Voting selection
- Officer operations
- Navigation and exit

The project contains separate keypad source, header and definition files.

---

📡 UART

UART0 provides communication between the RFID reader and LPC2148.

The RFID data is received through the UART interrupt mechanism.

The firmware waits for the complete RFID frame, copies the card ID and then compares it with the card IDs stored in EEPROM.

---

🔌 SPI

SPI0 is used for communication between the LPC2148 and the SPI EEPROM.

The project contains:

spi.c
spi.h
spi_defines.h
spi_eeprom.c
spi_eeprom.h
spi_eeprom_defines.h

These modules are included in the uploaded project structure.

---

🧩 Hardware Components

Based on the project source structure, the main hardware modules are:

Component| Purpose
LPC2148 ARM7| Main microcontroller
RFID Reader| Reads RFID card
RFID Card| Citizen identification
LCD| Displays information
4×4 Keypad| User input
AT25LC512 EEPROM| Non-volatile data storage
RTC| Date and time
Green LED| Valid card indication
Red LED| Invalid card indication
Buzzer| Invalid card indication

The RFID source specifically configures the indication outputs as GREEN LED = P0.19, RED LED = P0.20 and BUZZER = P0.21.

---

💻 Software / Development

Microcontroller

LPC2148 ARM7

Programming Language

Embedded C

Main File

main_rfid.c

Startup File

Startup.s

Communication Interfaces

UART
SPI

Storage

AT25LC512 SPI EEPROM

---

📂 Project Structure

RFID-based-unified-citizen-service-management-system/
│
├── headerfiles/
│   ├── defines.h
│   ├── delay.h
│   ├── kpm1.h
│   ├── lcd.h
│   ├── menu_rfid.h
│   ├── rfid.h
│   ├── rtc.h
│   ├── spi.h
│   ├── spi_eeprom.h
│   ├── types.h
│   └── uart.h
│
├── sourcefiles/
│   ├── delay.c
│   ├── kpm1.c
│   ├── lcd.c
│   ├── menu_rfid.c
│   ├── rfid.c
│   ├── rtc.c
│   ├── spi.c
│   ├── spi_eeprom.c
│   └── uart.c
│
├── Startup.s
│
└── main_rfid.c

The uploaded project archive lists these modules, including the RFID, menu, UART, RTC, SPI EEPROM, LCD, keypad and delay files.

---

🔄 Complete Project Flow

             POWER ON
                │
                ▼
       Initialize LPC2148
                │
       ┌────────┼─────────┐
       ▼        ▼         ▼
      UART     SPI       LCD
       │        │         │
       └────────┼─────────┘
                ▼
          Initialize RTC
                │
                ▼
        Check EEPROM
                │
       ┌────────┴────────┐
       │                 │
     Error              OK
       │                 │
       ▼                 ▼
   Stop System     Check Project
                  Initialization
                       │
                       ▼
                 Waiting Screen
                       │
                       ▼
                  Scan RFID
                       │
                       ▼
                Receive via UART
                       │
                       ▼
              Compare with EEPROM
                       │
          ┌────────────┼────────────┐
          │            │            │
       Invalid       User        Officer
          │            │            │
          ▼            ▼            ▼
     Red + Buzzer   Service      Officer
                    Menu          Menu
                       │            │
             ┌─────────┼────────┐   ├── Reset Votes
             │         │        │   └── Set DL Expiry
             ▼         ▼        ▼
           PAN        ATM      Voting
             │         │        │
             └─────────┼────────┘
                       ▼
                 Driving Licence
                       │
                       ▼
              EEPROM / RTC Data
                       │
                       ▼
                 Display Result
                       │
                       ▼
                 Back to Scan

---

⭐ Key Features

- RFID-based identification
- LPC2148 ARM7 embedded platform
- UART-based RFID communication
- SPI EEPROM data storage
- Password-protected services
- PAN Card information
- ATM balance, withdrawal and deposit
- One-vote-per-user voting
- NOTA option
- Driving Licence expiry checking
- RTC-based date/time management
- Officer administration card
- Vote reset facility
- Password change facility
- Valid card green LED indication
- Invalid card red LED + buzzer indication
- EEPROM initialization protection
- Automatic menu timeout
- "*" key for menu exit

The current menu implementation also uses a 10-second automatic timeout on applicable screens, while "*" can be used to exit screens immediately.

---

🛠️ Important Implementation Details

RFID Packet

Start Byte
    ↓
0x02
    ↓
8-byte RFID Card ID
    ↓
0x03

Card Verification

RFID Card
    ↓
UART
    ↓
LPC2148
    ↓
Read stored IDs from EEPROM
    ↓
Compare
    ↓
USER1 / USER2 / USER3 / OFFICER
             or
        INVALID CARD

Data Persistence

User Action
    ↓
Update Data
    ↓
Write to EEPROM
    ↓
Data remains available after restart

---

🚀 How to Use

1. Power ON the LPC2148 system.
2. Wait for the citizen service screen.
3. Scan a registered RFID card.
4. The system verifies the card.
5. For a valid user, the system displays the user's name.
6. Select a service using the keypad.
7. Enter the password when required.
8. Perform the selected operation.
9. Data is stored/updated in EEPROM where required.
10. Exit the service to return to the RFID waiting screen.

For an officer card, the system opens the officer administration menu.

---

🧪 Error Handling

The project includes handling for:

- Invalid RFID card
- EEPROM not connected
- Wrong password
- Three failed password attempts
- Insufficient ATM balance
- Invalid ATM amount
- Expired Driving Licence
- Already-voted user
- Invalid keypad input
- Menu timeout

---


📚 Modules

RFID Module
    ↓
UART Module
    ↓
LPC2148 Processing
    ↓
LCD + Keypad Interface
    ↓
Menu Management
    ↓
SPI EEPROM
    ↓
RTC
    ↓
Citizen Services

The repository separates these functions into dedicated source and header modules, making the project easier to maintain and understand.

---

🔮 Future Enhancements

Possible future improvements include:

- Fingerprint authentication
- Multiple RFID readers
- Secure encrypted citizen data
- Larger external storage
- Real government database integration
- Online service synchronization
- GSM/SMS notifications
- ESP32/IoT connectivity
- Web-based administration dashboard
- Cloud database integration
- Real-time transaction logging
- Biometric + RFID multi-factor authentication

---

👩‍💻 Author

Chevula Navya

B.Tech – Electronics and Communication Engineering



---
