/*
    Copyright 2014 David Fritz, Brian Gordon, Wira Mulia

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

#define OPR_NONE  0
#define OPR_HEX32 1
#define OPR_HEX8  2
#define OPR_DEC6  3
#define OPR_RANGE 4
#define OPR_ADVAL 5
#define OPR_BIT   6
#define OPR_BIT8  7
#define OPR_ADBIT 8

char* version = "jeruk-pic32-wload-alpha-1";
char* boot_info = "JERUK | http://plp.asu.edu";
char* cmd_error = "ERROR: Invalid or malformed command";
char input_buf[80];
char input_ptr;

char* help = "General operations:\n"
             "  help                 display this message\n"
             "  memory               list memory operations\n"
             "  u2                   list UART2 operations\n"
             "  port                 expansion port pinout information\n"
             "  reset                do a soft reset of the CPU\n"
             "  plpemu <addr>        invoke the on-chip PLP CPU emulator\n"
             "  rsw                  read switches\n"
             "  wled <val>           write LED values\n"
             "  rbtn                 read buttons 0 and 1\n";

char* help_uart2 =
             "Expansion Port UART2 operations:\n"
             "  u2tx <val>           send a byte through UART2\n"
             "  u2rx                 read a byte from UART2 (with timeout)\n"
             "  u2bd <baud>          set baud (decimal, 000000-xxxxxx, with leading zeroes)\n"
             "  u2pins               print UART2 pinout information\n"
             "  uart                 go into UART forward mode. Reset CPU to exit\n";

char* help_memory =
             "Memory operations:\n"
             "  wload                invoke the RAM loader\n"
             "  fload                invoke legacy RAM loader\n"
             "  wbyte <addr> <val>   write a byte to the specified address\n"
             "  rbyte <addr>         read a byte from the specified address\n"
             "  range <start> <end>  print byte values from specified address range\n"
             "  rword <start> <end>  likewise, in little-endian format\n"
             "  row <addr>           print 16 bytes of value starting from specified address\n"
             "  rowle <addr>         likewise, in little-endian format\n"
             "  ascii <start> <end>  print memory contents as ASCII characters\n"
             "  \nNotes:\n"
             "  Addresses must be entered as lowercase hex, e.g. a0004020\n"
             "  Byte values must be entered as a two-digit hex, e.g. a8\n"
             "    or an 8-bit value, e.g. 00110101\n";

char* u2pinout = "UART 2 Pins Designation (VDD=3.3V)\n\n"
                 "Header: P6\n"
                 "-------------------\n"
                 "1    2    3    4   \n"
                 "VSS  U2TX VDD  U2RX\n";

char* exppinout = "Expansion Port Pins Designation\n\n"
                  "Header: P5\n"
                  "-------------------------------------------------\n"
                  "IO7  IO6  IO5  IO4  IO3  IO2  IO1  IO0  3    1   \n"
                  "AN9  AN11 USB- USB+ SDA  SCL  INT0 RB4  3.3V VBUS\n"
                  "-------------------------------------------------\n"
                  "20   18   16   14   12   10   8    6    4    2\n"
                  "5V   5V   VSS  VSS  VSS  VSS  VSS  U2TX 3.3V U2RX\n";

void delay_ms(unsigned int);
void print_cpumodel(void);
void process_input(void);
void party(void);

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

    TRISB &= 0b1011111111110111; // pins RB3 and RB14 outputs for UART TX lines
    TRISA &= 0b1111111111100000; // LED pins

    // Disable ADC, for now
    AD1CON1bits.ADON = 0;
    ANSELA = 0;
    ANSELB = 0;

    BMXDKPBA = 0x4000; // 16k of RAM for data, rest for program
    BMXDUDBA = 0x10000;
    BMXDUPBA = 0x10000;

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

void main() {
    char buf;
    char wordbuf[9];

    jeruk_init();

    // we default into interactive mode
    pchar('\n');
    print(boot_info);
    print("\nFirmware: ");
    print(version);
    pchar('\n');
    ascii_hex_word(wordbuf, DEVID & 0x0fffffff);
    print("Microcontroller ID: ");
    psstr(wordbuf, 1, 7);
    print(" (");
    print_cpumodel();
    print(") Rev: ");
    pchar(ascii_byte_l((char)((DEVID & 0xf0000000) >> 28)));
    print("\n> ");
    input_ptr = 0;

    while(1) {
        buf = blocking_read();
        if(input_ptr == 79 || buf == 0x0d) {
            process_input();
            input_ptr = 0;
        } else {
            pchar(buf);
            input_buf[input_ptr] = buf;
            input_ptr++;
        }
    }
}

char parse(char* cmd, char opr_type) {
    int i  = 0;
    char buf;
    int cmd_len = 0;
    while(cmd[cmd_len] != 0) {
        cmd_len++;
    }

    while((buf = cmd[i]) != 0 && i < input_ptr) {
        if(input_buf[i] != buf) {
            return 0;
        }
        i++;
    }

    if(opr_type == OPR_NONE && cmd_len != input_ptr) {
        return 0;
    }

    else if((opr_type == OPR_HEX32 || opr_type == OPR_BIT8) && (cmd_len+1+8) != input_ptr) {
        return 0;
    }

    else if(opr_type == OPR_HEX8 && (cmd_len+1+2) != input_ptr) {
        return 0;
    }

    else if(opr_type == OPR_BIT && (cmd_len+1+1) != input_ptr) {
        return 0;
    }

    else if(opr_type == OPR_DEC6 && (cmd_len+1+6) != input_ptr) {
        return 0;
    }

    else if((opr_type == OPR_RANGE || opr_type == OPR_ADBIT) && (cmd_len+1+8+1+8) != input_ptr) {
        return 0;
    }

    else if(opr_type == OPR_ADVAL && (cmd_len+1+8+1+2) != input_ptr) {
        return 0;
    }

    if(opr_type == OPR_HEX32 || opr_type == OPR_HEX8) {
        if(input_buf[i] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr; i++) {
            if(!((input_buf[i] >= '0' && input_buf[i] <= '9') ||
               (input_buf[i] >= 'a' && input_buf[i] <= 'f'))) {
                return 0;
            }
        }
    }

    else if(opr_type == OPR_RANGE) {
        if(input_buf[i] != ' ' || input_buf[input_ptr-9] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr-9; i++) {
            if(!((input_buf[i] >= '0' && input_buf[i] <= '9') ||
               (input_buf[i] >= 'a' && input_buf[i] <= 'f'))) {
                return 0;
            }
        }

        for(i=input_ptr-8; i < input_ptr; i++) {
            if(!((input_buf[i] >= '0' && input_buf[i] <= '9') ||
               (input_buf[i] >= 'a' && input_buf[i] <= 'f'))) {
                return 0;
            }
        }
    }

    else if(opr_type == OPR_ADVAL) {
        if(input_buf[i] != ' ' || input_buf[input_ptr-3] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr-3; i++) {
            if(!((input_buf[i] >= '0' && input_buf[i] <= '9') ||
               (input_buf[i] >= 'a' && input_buf[i] <= 'f'))) {
                return 0;
            }
        }

        for(i=input_ptr-2; i < input_ptr; i++) {
            if(!((input_buf[i] >= '0' && input_buf[i] <= '9') ||
               (input_buf[i] >= 'a' && input_buf[i] <= 'f'))) {
                return 0;
            }
        }
    }

    else if(opr_type == OPR_ADBIT) {
        if(input_buf[i] != ' ' || input_buf[input_ptr-9] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr-9; i++) {
            if(!((input_buf[i] >= '0' && input_buf[i] <= '9') ||
               (input_buf[i] >= 'a' && input_buf[i] <= 'f'))) {
                return 0;
            }
        }

        for(i=input_ptr-8; i < input_ptr; i++) {
            if(input_buf[i] != '0' && input_buf[i] != '1') {
                return 0;
            }
        }
    }

    else if(opr_type == OPR_DEC6) {
        if(input_buf[i] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr; i++) {
            if(!(input_buf[i] >= '0' && input_buf[i] <= '9')) {
                return 0;
            }
        }
    }

    else if(opr_type == OPR_BIT || opr_type == OPR_BIT8) {
        if(input_buf[i] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr; i++) {
            if(input_buf[i] != '0' && input_buf[i] != '1') {
                return 0;
            }
        }
    }

    return 1;
}

void cmd_range() {
    char wordbuf[9];
    int i;
    char* ptr_byte = (char*) parse_ascii_hex_32(input_buf, 6);
    char* ptr_byte_end = (char*) parse_ascii_hex_32(input_buf, 15);

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
        pchar('\n');
    }
}

void cmd_row() {
    int i;
    char* ptr_byte = (char*) parse_ascii_hex_32(input_buf, 4);
    psstr(input_buf, 4, 8);
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
    char* ptr_byte = (char*) parse_ascii_hex_32(input_buf, 6);
    char* ptr_byte_end = (char*) parse_ascii_hex_32(input_buf, 15);

    print("Words are displayed as little-endian\n");

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
        pchar('\n');
    }
}

void cmd_rowle() {
    int i, j;
    char wordbuf[9];
    char* ptr_byte = (char*) parse_ascii_hex_32(input_buf, 6);

    print("Words are displayed as little-endian\n");

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
    pchar('\n');
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

void cmd_wled(char hex) {
    char val;
    if(hex) {
        val = parse_ascii_hex_byte(input_buf, 5);
    } else {
        val = parse_ascii_bit_8(input_buf, 5);
    }
    LEDS = val;
}

void cmd_rbyte() {
    char* ptr_byte = (char*) parse_ascii_hex_32(input_buf, 6);
    print("Value: ");
    pchar(ascii_byte_h(*ptr_byte));
    pchar(ascii_byte_l(*ptr_byte));
}

void cmd_wbyte(char hex) {
    char val;
    char* ptr_byte = (char*) parse_ascii_hex_32(input_buf, 6);
    if(hex) {
        val = parse_ascii_hex_byte(input_buf, 15);
    } else {
        val = parse_ascii_bit_8(input_buf, 15);
    }
    *ptr_byte = val;
}

void cmd_ascii() {
    int i;
    char wordbuf[9];
    char* ptr_byte = (char*) parse_ascii_hex_32(input_buf, 6);
    char* ptr_byte_end = (char*) parse_ascii_hex_32(input_buf, 15);

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
        pchar('\n');
    }
}

void process_input() {

    print("\n");

         if(parse("help",   OPR_NONE))   print(help);
    else if(parse("memory", OPR_NONE))   print(help_memory);
    else if(parse("port",   OPR_NONE))   print(exppinout);
    else if(parse("reset",  OPR_NONE))   SoftReset();
    else if(parse("plpemu", OPR_HEX32))  emu_plp5(parse_ascii_hex_32(input_buf, 7));
    else if(parse("rsw",    OPR_NONE))   cmd_rsw();
    else if(parse("rbtn",   OPR_NONE))   cmd_rbtn();
    else if(parse("wled",   OPR_HEX8))   cmd_wled(1);
    else if(parse("wled",   OPR_BIT8))   cmd_wled(0);
    else if(parse("wload",  OPR_NONE))   wload();
    else if(parse("fload",  OPR_NONE))   fload();
    else if(parse("party",  OPR_NONE))   party();

    else if(parse("u2",     OPR_NONE))   print(help_uart2);
    else if(parse("u2pins", OPR_NONE))   print(u2pinout);
    else if(parse("u2rx",   OPR_NONE))   u2_read_print();
    else if(parse("u2tx",   OPR_HEX8))   u2_write(parse_ascii_hex_byte(input_buf, 5));
    else if(parse("u2bd",   OPR_DEC6))   u2_set_baud(parse_ascii_decimal(input_buf, 5, 6));
    else if(parse("uart",   OPR_NONE))   uart_forward();

    else if(parse("wbyte",  OPR_ADVAL))  cmd_wbyte(1);
    else if(parse("wbyte",  OPR_ADBIT))  cmd_wbyte(0);
    else if(parse("rbyte",  OPR_HEX32))  cmd_rbyte();
    else if(parse("range",  OPR_RANGE))  cmd_range();
    else if(parse("rword",  OPR_RANGE))  cmd_rword();
    else if(parse("rowle",  OPR_HEX32))  cmd_rowle();
    else if(parse("row",    OPR_HEX32))  cmd_row();
    else if(parse("ascii",  OPR_RANGE))  cmd_ascii();

    else print(cmd_error);

    print("\n> ");
}

// should be pretty close enough, 4 instructions / loop
void delay_ms(unsigned int milliseconds) {
    unsigned int t = milliseconds * (SYSTEM_CLOCK / 1000 / 4);
    __asm__("   move $t0, %0                 \n"
            "wdelay_loop:                    \n"
            "   beq $t0, $zero, wdelay_quit  \n"
            "   addiu $t0, $t0, -1           \n"
            "   j wdelay_loop                \n"
            "   nop                          \n"
            "wdelay_quit:                    \n"
            : : "r"(t) : "t0"
            );
}

// print the (supported) CPU model the firmware is running on
void print_cpumodel() {
    int val = DEVID & 0x0fffffff;
    if(val == 0x6600053)         print("PIC32MX270F256B");
    else if(val == 0x4D00053)    print("PIC32MX250F128B");
    else if(val == 0x4D01053)    print("PIC32MX230F064B");
    else                         print("Unknown");    
}

void party() {
    char stop = 0;
    while(!stop) {
        if(U1STAbits.URXDA) {
            if(U1RXREG == 'q') {
                stop = 1;
            }
        }
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
        LEDS = 0;
        delay_ms(50);
        LEDS = 0b10001;
        delay_ms(50);
        LEDS = 0b11011;
        delay_ms(50);
        LEDS = 0b11111;
        delay_ms(50);
        LEDS = 0b01110;
        delay_ms(50);
        LEDS = 0b00100;
        delay_ms(50);
        LEDS = 0;
    }
}

