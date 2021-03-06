/*
    Copyright 2014-2015 David Fritz, Brian Gordon, Wira Mulia

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#include <xc.h>
#include <proc/p32mx270f256b.h>
#include "wio.h"
#include "wloader.h"
#include "plpemu.h"
#include "proccfg.h"
#include "procdefs.h"
#include "envy.h"
#include "spi.h"
#include "misc.h"

char* version = "jeruk-pic32-wload-alpha-2";
char* boot_info = "JERUK | http://plp.asu.edu";
char* copyright = "Copyright (c)2014-2015 PLP Contributors";

char* help = "General operations:\n\r"
             "  help                 display this message\n\r"
             "  memory               list memory operations\n\r"
             "  u2                   list UART2 operations\n\r"
             "  port                 expansion port pinout information\n\r"
             "  reset                do a soft reset of the CPU\n\r"
             "  plpemu <addr>        invoke the on-chip PLP CPU emulator\n\r"
             "  envy                 invoke the flash loader\n\r"
             "  rsw                  read switches\n\r"
             "  wled <val>           write LED values\n\r"
             "  rbtn                 read buttons 0 and 1\n\r";

char* help_uart2 =
             "Expansion Port UART2 operations:\n\r"
             "  u2tx <val>           send a byte through UART2\n\r"
             "  u2rx                 read a byte from UART2 (with timeout)\n\r"
             "  u2bd <baud>          set baud (decimal, 000000-xxxxxx, with leading zeroes)\n\r"
             "  u2pins               print UART2 pinout information\n\r"
             "  uart                 go into UART forward mode. Reset CPU to exit\n\r";

char* help_memory =
             "Memory operations:\n\r"
             "  wload                invoke the RAM loader\n\r"
             "  fload                invoke legacy RAM loader\n\r"
             "  wbyte <addr> <val>   write a byte to the specified address\n\r"
             "  rbyte <addr>         read a byte from the specified address\n\r"
             "  range <start> <end>  print byte values from specified address range\n\r"
             "  rword <start> <end>  likewise, in little-endian format\n\r"
             "  row <addr>           print 16 bytes of value starting from specified address\n\r"
             "  rowle <addr>         likewise, in little-endian format\n\r"
             "  ascii <start> <end>  print memory contents as ASCII characters\n\r"
             "  \n\rNotes:\n\r"
             "  Addresses must be entered as lowercase hex, e.g. a0004020\n\r"
             "  Byte values must be entered as a two-digit hex, e.g. a8\n\r"
             "    or an 8-bit value, e.g. 00110101\n\r";

char* u2pinout = "UART 2 Pins Designation (VDD=3.3V)\n\r\n\r"
                 "Header: P6\n\r"
                 "-------------------\n\r"
                 "1    2    3    4   \n\r"
                 "VSS  U2TX VDD  U2RX\n\r";

char* exppinout = "Expansion Port Pins Designation\n\r\n\r"
                  "Header: P5\n\r"
                  "-------------------------------------------------\n\r"
                  "IO7  IO6  IO5  IO4  IO3  IO2  IO1  IO0  3    1   \n\r"
                  "AN9  AN11 USB- USB+ SDA  SCL  INT0 RB4  3.3V VBUS\n\r"
                  "-------------------------------------------------\n\r"
                  "20   18   16   14   12   10   8    6    4    2\n\r"
                  "5V   5V   VSS  VSS  VSS  VSS  VSS  U2TX 3.3V U2RX\n\r";

char* cmd_error = "ERROR: Invalid or malformed command";

void jeruk_init(void);
void delay_ms(unsigned int);
void print_cpumodel(void);
void process_input(void);
void party(void);

void main() {
    char wordbuf[9];

    jeruk_init();

    // we default into interactive mode
    pnewl();
    print(boot_info);
    pnewl();
    print(copyright);
    print("\n\rFirmware: ");
    print(version);
    pnewl();
    ascii_hex_word(wordbuf, DEVID & 0x0fffffff);
    print("Microcontroller ID: ");
    psstr(wordbuf, 1, 7);
    print(" (");
    print_cpumodel();
    print(") Rev: ");
    pchar(ascii_byte_l((char)((DEVID & 0xf0000000) >> 28)));
    print("\n\r> ");

    while(1) {
        wio_readline();
        process_input();
    }
}

 // JERUK board initialization
void jeruk_init() {
    // Setup UART pin-mapping
    U1RXR  = 0b0100;    // U1RX mapped to RB2
    U2RXR  = 0b0001;    // U2RX mapped to RB5
    RPB3R  = 0b0001;    // U1TX mapped to RB3
    RPB14R = 0b0010;    // U2TX mapped to RB14

    // Setup UARTs
    init_uart1(SYSTEM_CLOCK, UART1_BAUD);
    init_uart2(SYSTEM_CLOCK, UART2_BAUD);
    
    // Set terminal options
    WIO_EMULATE_VT = 1;
    WIO_BACKSPACE_SUPPORT = 1;

    TRISB &= 0b1011111111110111; // pins RB3 and RB14 outputs for UART TX lines
    TRISA &= 0b1111111111100000; // LED pins

    // Disable ADC, for now
    AD1CON1bits.ADON = 0;
    ANSELA = 0;
    ANSELB = 0;

    //BMXDKPBA = 0x4000; // 16k of RAM for data, rest for program
    //BMXDUDBA = 0x10000;
    //BMXDUPBA = 0x10000;

    LEDS = 0b00100;
    delay_ms(50);
    LEDS = 0b01110;
    delay_ms(50);
    LEDS = 0b11111;
    delay_ms(50);
    LEDS = 0b11011;
    delay_ms(50);
    LEDS = 0b10001;
    delay_ms(50);
    if(BTN0 && !BTN1) {
        LEDS = 0b10000; // fload time!
        fload();
    } else {
        LEDS = 0;
    }
}

void cmd_range() {
    char wordbuf[9];
    int i;
    char* ptr_byte = (char*) wio_opr_int1;
    char* ptr_byte_end = (char*) wio_opr_int2;

    while(ptr_byte < ptr_byte_end) {
        ascii_hex_word(wordbuf, (int) ptr_byte);
        print(wordbuf);
        pchar(' ');
        for(i = 0; i < 16; i++) {
            if(i % 4 == 0) {
                pchar(' ');
            }
            pchar(ascii_byte_h(*ptr_byte));
            pchar(ascii_byte_l(*ptr_byte));
            pchar(' ');
            ptr_byte++;
        }
        pnewl();
    }
}

void cmd_row() {
    int i;
    char* ptr_byte = (char*) wio_opr_int1;
    psstr(wio_line.data, 4, 8);
    pchar(' ');
    
    for(i = 0; i < 16; i++) {
        if(i % 4 == 0) {
            pchar(' ');
        }
        pchar(ascii_byte_h(*ptr_byte));
        pchar(ascii_byte_l(*ptr_byte));
        pchar(' ');
        ptr_byte++;
    }
}

void cmd_rword() {
    int i, j;
    char wordbuf[9];
    char* ptr_byte = (char*) wio_opr_int1;
    char* ptr_byte_end = (char*) wio_opr_int2;

    print("Words are displayed as little-endian\n\r");

    while(ptr_byte < ptr_byte_end) {
        ascii_hex_word(wordbuf, (int) ptr_byte);
        print(wordbuf);
        pchar(' ');
        for(i = 0; i < 4; i++) {
            pchar(' ');
            for(j = 3; j >= 0; j--) {
                pchar(ascii_byte_h(*(ptr_byte+j)));
                pchar(ascii_byte_l(*(ptr_byte+j)));
                pchar(' ');
            }
            ptr_byte += 4;
        }
        pnewl();
    }
}

void cmd_rowle() {
    int i, j;
    char wordbuf[9];
    char* ptr_byte = (char*) wio_opr_int1;

    print("Words are displayed as little-endian\n\r");

    ascii_hex_word(wordbuf, (int) ptr_byte);
    print(wordbuf);
    pchar(' ');
    for(i = 0; i < 4; i++) {
        pchar(' ');
        for(j = 3; j >= 0; j--) {
            pchar(ascii_byte_h(*(ptr_byte+j)));
            pchar(ascii_byte_l(*(ptr_byte+j)));
            pchar(' ');
        }
        ptr_byte += 4;
    }
    pnewl();
}

void cmd_rsw() {
    char val;
    print("Switch values: ");
    val = 0 | IO_7 << 7 | IO_6 << 6 | IO_5 << 5 | IO_4 << 4 |
              IO_3 << 3 | IO_2 << 2 | IO_1 << 1 | IO_0;
    pchar(ascii_byte_h(val));
    pchar(ascii_byte_l(val));
}

void cmd_rbtn() {
    char val = PORTB & 0x3;
    print("Button values: ");
    pchar(ascii_byte_l(val));
}

void cmd_wled() {
    char val = wio_opr_char;
    LEDS = val;
}

void cmd_rbyte() {
    char* ptr_byte = (char*) wio_opr_int1;
    print("Value: ");
    pchar(ascii_byte_h(*ptr_byte));
    pchar(ascii_byte_l(*ptr_byte));
}

void cmd_wbyte() {
    char val = wio_opr_char;
    char* ptr_byte = (char*) wio_opr_int1;
    *ptr_byte = val;
}

void cmd_ascii() {
    int i;
    char wordbuf[9];
    char* ptr_byte = (char*) wio_opr_int1;
    char* ptr_byte_end = (char*) wio_opr_int2;

    while(ptr_byte < ptr_byte_end) {
        ascii_hex_word(wordbuf, (int) ptr_byte);
        print(wordbuf);
        pchar(' ');
        for(i = 0; i < 16; i++) {
            if(i % 4 == 0) {
                pchar(' ');
            }
            if(*ptr_byte >= 32 && *ptr_byte <= 126) {
                pchar(*ptr_byte);
            } else {
                pchar('.');
            }
            pchar(' ');
            ptr_byte++;
        }
        pnewl();
    }
}

void process_input() {

    print("\n\r");

         if(parse("help",   OPR_NONE))   print(help);
    else if(parse("memory", OPR_NONE))   print(help_memory);
    else if(parse("port",   OPR_NONE))   print(exppinout);
    else if(parse("reset",  OPR_NONE))   SoftReset();
    else if(parse("plpemu", OPR_HEX32))  emu_plp5(wio_opr_int1);
    else if(parse("rsw",    OPR_NONE))   cmd_rsw();
    else if(parse("rbtn",   OPR_NONE))   cmd_rbtn();
    else if(parse("wled",   OPR_HEX8))   cmd_wled();
    else if(parse("wled",   OPR_BIN8))   cmd_wled();
    else if(parse("wload",  OPR_NONE))   wload();
    else if(parse("fload",  OPR_NONE))   fload();
    else if(parse("envy",   OPR_NONE))   envy();
    else if(parse("envycl", OPR_NONE))   envy_clear();
    else if(parse("spi",    OPR_NONE))   spi();
    else if(parse("party",  OPR_NONE))   party();
    else if(parse("btuart", OPR_NONE))   button_uart('a', 'b');

    else if(parse("u2",     OPR_NONE))   print(help_uart2);
    else if(parse("u2pins", OPR_NONE))   print(u2pinout);
    else if(parse("u2rx",   OPR_NONE))   u2_read_print();
    else if(parse("u2tx",   OPR_HEX8))   u2_write(wio_opr_char);
    else if(parse("u2bd",   OPR_DEC6))   u2_set_baud(wio_opr_int1);
    else if(parse("uart",   OPR_NONE))   uart_forward();

    else if(parse("wbyte",  OPR_ADVAL))  cmd_wbyte();
    else if(parse("wbyte",  OPR_ADBIT))  cmd_wbyte();
    else if(parse("rbyte",  OPR_HEX32))  cmd_rbyte();
    else if(parse("range",  OPR_RANGE))  cmd_range();
    else if(parse("rword",  OPR_RANGE))  cmd_rword();
    else if(parse("rowle",  OPR_HEX32))  cmd_rowle();
    else if(parse("row",    OPR_HEX32))  cmd_row();
    else if(parse("ascii",  OPR_RANGE))  cmd_ascii();

    else print(cmd_error);

    print("\n\r> ");
}

// print the (supported) CPU model the firmware is running on
void print_cpumodel() {
    int val = DEVID & 0x0fffffff;
    if(val == 0x6600053)         print("PIC32MX270F256B");
    else if(val == 0x4D00053)    print("PIC32MX250F128B");
    else if(val == 0x4D01053)    print("PIC32MX230F064B");
    else                         print("Unknown");    
}

