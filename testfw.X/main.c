/* 
 * File:   main.c
 * Author: Wira
 *
 * Created on October 28, 2014, 4:00 PM
 */

#include <xc.h>
#include <proc/p32mx270f256b.h>
#include "wio.h"
#include "wloader.h"
#include "plpemu.h"

// DEVCFG3
// USERID = No Setting
#pragma config PMDL1WAY = ON            // Peripheral Module Disable Configuration (Allow only one reconfiguration)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow only one reconfiguration)
#pragma config FUSBIDIO = OFF           // USB USID Selection (Controlled by Port Function)
#pragma config FVBUSONIO = OFF          // USB VBUS ON Selection (Controlled by Port Function)

// DEVCFG2
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_24         // PLL Multiplier (24x Multiplier)
#pragma config UPLLIDIV = DIV_12        // USB PLL Input Divider (12x Divider)
#pragma config UPLLEN = OFF             // USB PLL Enable (Disabled and Bypassed)
#pragma config FPLLODIV = DIV_2         // System PLL Output Clock Divider (PLL Divide by 2), effective clock is 48MHz (8*24/2/2)

// DEVCFG1
#pragma config FNOSC = FRCPLL           // Oscillator Selection Bits (Fast RC Osc with PLL)
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Configuration (Primary osc disabled)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_1           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/1) pbclock is 48MHz
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config WINDIS = OFF             // Watchdog Timer Window Enable (Watchdog Timer is in Non-Window Mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))
#pragma config FWDTWINSZ = WINSZ_25     // Watchdog Timer Window Size (Window Size is 25%)

// DEVCFG0
#pragma config JTAGEN = OFF             // JTAG Enable (JTAG Port Enabled)
#pragma config ICESEL = ICS_PGx1        // ICE/ICD Comm Channel Select (Communicate on PGEC1/PGED1)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

#define IO_0 PORTBbits.RB4
#define IO_1 PORTBbits.RB7
#define IO_2 PORTBbits.RB8
#define IO_3 PORTBbits.RB9
#define IO_4 PORTBbits.RB10
#define IO_5 PORTBbits.RB11
#define IO_6 PORTBbits.RB13
#define IO_7 PORTBbits.RB15

#define BTN0 PORTBbits.RB0
#define BTN1 PORTBbits.RB1

#define SYSTEM_CLOCK    48000000
#define UART1_BAUD      115200
#define UART2_BAUD      9600
#define UART_TIMEOUT    7200000L

#define LEDS PORTA

char* version = "jeruk-pic32-wload-alpha-1";
char* boot_info = "JERUK | http://plp.asu.edu";
char* cmd_error = "ERROR: Invalid or malformed command";
char input_buf[80];
char input_ptr;

char* cmd_rsw = "rsw";
char* cmd_rbtn = "rbtn";
char* cmd_help = "help";
char* cmd_wled = "wled";
char* cmd_party = "party";
char* cmd_w32 = "w32";
char* cmd_r32 = "r32";
char* cmd_rbyte = "rbyte";
char* cmd_wbyte = "wbyte";
char* cmd_org = "org";
char* cmd_range = "range";
char* cmd_rword = "rword";
char* cmd_ascii = "ascii";
char* cmd_reset = "reset";
char* cmd_wload = "wload";
char* cmd_fload = "fload";
char* cmd_plpemu = "plpemu";
char* cmd_u2tx = "u2tx";
char* cmd_u2rx = "u2rx";
char* cmd_uart = "uart";
char* cmd_u2bd = "u2bd";
char* cmd_u2pins = "u2pins";
char* cmd_port = "port";

char* help = "General operations:\n"
             "  help                 display this message\n"
             "  port                 expansion port pinout information\n"
             "  reset                do a soft reset of the CPU\n"
             "  plpemu <addr>        invoke the on-chip PLP CPU emulator\n"
             "  rsw                  read switches\n"
             "  wled <val>           write LED values\n"
             "  rbtn                 read buttons 0 and 1\n"
             "\nExpansion Port UART operations:\n"
             "  u2tx <val>           send a byte through UART 2\n"
             "  u2rx                 read a byte from UART 2 (with timeout)\n"
             "  u2bd <baud>          set baud (in decimal, 000000-115200)\n"
             "  u2pins               print UART 2 pinout information\n"
             "  uart                 go into UART forward mode. Reset CPU to exit\n"
             "\nMemory operations:\n"
             "  wload                invoke the RAM loader\n"
             "  fload                invoke legacy RAM loader\n"
             "  wbyte <addr> <val>   write a byte to the specified address\n"
             "  rbyte <addr>         read a byte from the specified address\n"
             "  range <start> <end>  print byte values from specified address range\n"
             "  rword <start> <end>  likewise, in little-endian format\n"
             "  range <addr>         print 16 bytes of value starting from specified address\n"
             "  rword <addr>         likewise, in little-endian format\n"
             "  ascii <start> <end>  print memory contents as ASCII characters\n"
             "  \nNotes:\n"
             "  Addresses must be entered as lowercase hex, e.g. a0004020\n"
             "  Byte values must be entered as a two-digit hex, e.g. a8\n";

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

int programmer_pc;

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
    if(val == 0x6600053) {          print("PIC32MX270F256B");
    } else if(val == 0x4D00053) {   print("PIC32MX250F128B");
    } else if(val == 0x4D01053) {   print("PIC32MX230F064B");
    } else                      {   print("Unknown");
    }
}

void process_input(void);
void party(void);

void main() {
    // JERUK board initialization

    // Setup UART pin-mapping
    U1RXR  = 0b0100;    // U1RX mapped to RB2
    U2RXR  = 0b0001;    // U2RX mapped to RB5
    RPB3R  = 0b0001;    // U1TX mapped to RB3
    RPB14R = 0b0010;    // U2TX mapped to RB14

    // Setup UART 1
    U1MODEbits.ON = 1;          // on
    U1MODEbits.UEN = 0b00;      // rx/tx pins only
    U1MODEbits.BRGH = 1;        // brgh mode
    U1MODEbits.PDSEL = 0b00;    // 8-bit data, no parity
    U1MODEbits.STSEL = 0;       // 1 stop bit
    U1STAbits.URXEN = 1;        // receive enable
    U1STAbits.UTXEN = 1;        // transmit enable
    U1BRG = (SYSTEM_CLOCK/UART1_BAUD)/4 - 1;

    // Setup UART 2
    U2MODEbits.ON = 1;          // on
    U2MODEbits.UEN = 0b00;      // rx/tx pins only
    U2MODEbits.BRGH = 1;        // brgh mode
    U2MODEbits.PDSEL = 0b00;    // 8-bit data, no parity
    U2MODEbits.STSEL = 0;       // 1 stop bit
    U2STAbits.URXEN = 1;        // receive enable
    U2STAbits.UTXEN = 1;        // transmit enable
    U2BRG = (SYSTEM_CLOCK/UART2_BAUD)/4 - 1;

    TRISB &= 0b1011111111110111; // pins RB3 and RB14 outputs for UART TX lines
    TRISA &= 0b1111111111100000; // LED pins

    RPB4R = 0;
    RPB9R = 0;

    // Disable ADC, for now
    AD1CON1bits.ADON = 0;
    ANSELA = 0;
    ANSELB = 0;

    BMXDKPBA = 0x4000; // 16k of RAM for data, rest for program
    BMXDUDBA = 0x10000;
    BMXDUPBA = 0x10000;

    char buf;
    char wordbuf[9];

    programmer_pc = 0;

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

    // we default into interactive mode
    pchar('\n');
    print(boot_info);
    print("\nFirmware: ");
    print(version);
    pchar('\n');
    ascii_hex_word(wordbuf, DEVID & 0x0fffffff);
    print("Microcontroller ID: ");
    print(wordbuf);
    print(" (");
    print_cpumodel();
    print(") Rev: ");
    pchar(ascii_byte_l((char)((DEVID & 0xf0000000) >> 28)));
    print("\n> ");
    input_ptr = 0;

    while(1) {
        // echo input
        if(U1STAbits.URXDA && U1STAbits.TRMT) {
            buf = U1RXREG;
            if(input_ptr == 80 || buf == 0x0d) {
                process_input();
                input_ptr = 0;
            } else {
                U1TXREG = buf;
                input_buf[input_ptr] = buf;
                input_ptr++;
            }
        }
    }
}

void process_input() {
    char val;
    char* ptr_byte;
    char* ptr_byte_end;
    char wordbuf[9];
    int i, j;
    unsigned int timeout_count;
    unsigned int val_int;

    print("\n");

    if(input_ptr == 3) {
        // check read switch
        if(str_cmp(cmd_rsw, input_buf, 3)) {
            print("Switch values: ");
            val = 0 | IO_7 << 7 | IO_6 << 6 | IO_5 << 5 | IO_4 << 4 |
                      IO_3 << 3 | IO_2 << 2 | IO_1 << 1 | IO_0;
            pchar(ascii_byte_h(val));
            pchar(ascii_byte_l(val));
        } else {
            print(cmd_error);
        }
    } else if(input_ptr == 4) {
        if(str_cmp(cmd_help, input_buf, 4)) {
            print(help);
        } else if(str_cmp(cmd_rbtn, input_buf, 4)) {
            val = PORTB & 0x3;
            print("Button values: ");
            pchar(ascii_byte_l(val));
        } else if(str_cmp(cmd_u2rx, input_buf, 4)) {
            timeout_count = 0;
            while(timeout_count < UART_TIMEOUT && !U2STAbits.URXDA) {
                timeout_count++;
            }
            if(U2STAbits.URXDA) {
                val = U2RXREG;
                pchar(ascii_byte_h(val));
                pchar(ascii_byte_l(val));
            } else {
                print("UART receive timeout\n");
            }
        } else if(str_cmp(cmd_uart, input_buf, 4)) {
            uart_forward();
        } else if(str_cmp(cmd_port, input_buf, 4)) {
            print(exppinout);
        }else {
            print(cmd_error);
        }
    } else if(input_ptr == 5) {
        if(str_cmp(cmd_reset, input_buf, 5)) {
            SoftReset();
        } else if(str_cmp(cmd_wload, input_buf, 5)) {
            wload();
        } else if(str_cmp(cmd_fload, input_buf, 5)) {
            fload();
        } else if(str_cmp(cmd_party, input_buf, 5)) {
            party();
        } else {
            print(cmd_error);
        }
    } else if(input_ptr == 6) {
        if(str_cmp(cmd_u2pins, input_buf, 6)) {
            print(u2pinout);
        } else {
            print(cmd_error);
        }
    }else if(input_ptr == 7) {
        if(str_cmp(cmd_wled, input_buf, 4)) {
            val = parse_ascii_hex_byte(input_buf[5], input_buf[6]);
            LEDS = val;
        } else if(str_cmp(cmd_u2tx, input_buf, 4)) {
            val = parse_ascii_hex_byte(input_buf[5], input_buf[6]);
            while(!U2STAbits.TRMT);
            U2TXREG = val;
        } else {
            print(cmd_error);
        }
    } else if(input_ptr == 11) {
        if(str_cmp(cmd_u2bd, input_buf, 4)) {
            val_int = parse_ascii_decimal(input_buf, 5, 6);
            U2BRG = (48000000/val_int)/4 - 1;
            print("New baud: ");
            ascii_hex_word(wordbuf, val_int);
            print(wordbuf);
            pchar('\n');
            print("brg value: ");
            ascii_hex_word(wordbuf, U2BRG);
            print(wordbuf);
            pchar('\n');
        } else {
            print(cmd_error);
        }
    } else if(input_ptr == 14) {
        if(str_cmp(cmd_range, input_buf, 5)) {
            psstr(input_buf, 6, 8);
            pchar(' ');
            ptr_byte = (char*) parse_ascii_hex_word(input_buf[6], input_buf[7], input_buf[8], input_buf[9],
                    input_buf[10], input_buf[11], input_buf[12], input_buf[13]);
            for(i = 0; i < 16; i++) {
                if(i % 4 == 0) {
                    pchar(' ');
                }
                pchar(ascii_byte_h(*ptr_byte));
                pchar(ascii_byte_l(*ptr_byte));
                pchar(' ');
                ptr_byte++;
            }
        } else if(str_cmp(cmd_rbyte, input_buf, 5)) {
            ptr_byte = (char*) parse_ascii_hex_word(input_buf[6], input_buf[7], input_buf[8], input_buf[9],
                    input_buf[10], input_buf[11], input_buf[12], input_buf[13]);
            print("Value: ");
            pchar(ascii_byte_h(*ptr_byte));
            pchar(ascii_byte_l(*ptr_byte));
        } else if(str_cmp(cmd_rword, input_buf, 5)) {
            print("Words are displayed as little-endian\n");
            ptr_byte = (char*) parse_ascii_hex_word(input_buf[6], input_buf[7], input_buf[8], input_buf[9],
                    input_buf[10], input_buf[11], input_buf[12], input_buf[13]);
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
        } else {
            print(cmd_error);
        }
    } else if(input_ptr == 15) {
        if(str_cmp(cmd_plpemu, input_buf, 6)) {
            emu_plp5(parse_ascii_hex_word(input_buf[7], input_buf[8], input_buf[9], input_buf[10],
                     input_buf[11], input_buf[12], input_buf[13], input_buf[14]));
        } else {
            print(cmd_error);
        }
    } else if(input_ptr == 17) {
        if(str_cmp(cmd_wbyte, input_buf, 5)) {
            ptr_byte = (char*) parse_ascii_hex_word(input_buf[6], input_buf[7], input_buf[8], input_buf[9],
                    input_buf[10], input_buf[11], input_buf[12], input_buf[13]);
            val = parse_ascii_hex_byte(input_buf[15], input_buf[16]);
            *ptr_byte = val;
        } else {
            print(cmd_error);
        }
    } else if(input_ptr == 23) {
        if(str_cmp(cmd_range, input_buf, 5)) {
            ptr_byte = (char*) parse_ascii_hex_word(input_buf[6], input_buf[7], input_buf[8], input_buf[9],
                    input_buf[10], input_buf[11], input_buf[12], input_buf[13]);
            ptr_byte_end = (char*) parse_ascii_hex_word(input_buf[15], input_buf[16], input_buf[17], input_buf[18],
                    input_buf[19], input_buf[20], input_buf[21], input_buf[22]);

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
        } else if(str_cmp(cmd_rword, input_buf, 5)) {
            print("Words are displayed as little-endian\n");
            ptr_byte = (char*) parse_ascii_hex_word(input_buf[6], input_buf[7], input_buf[8], input_buf[9],
                    input_buf[10], input_buf[11], input_buf[12], input_buf[13]);
            ptr_byte_end = (char*) parse_ascii_hex_word(input_buf[15], input_buf[16], input_buf[17], input_buf[18],
                    input_buf[19], input_buf[20], input_buf[21], input_buf[22]);

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
        } else if(str_cmp(cmd_ascii, input_buf, 5)) {
            ptr_byte = (char*) parse_ascii_hex_word(input_buf[6], input_buf[7], input_buf[8], input_buf[9],
                    input_buf[10], input_buf[11], input_buf[12], input_buf[13]);
            ptr_byte_end = (char*) parse_ascii_hex_word(input_buf[15], input_buf[16], input_buf[17], input_buf[18],
                    input_buf[19], input_buf[20], input_buf[21], input_buf[22]);

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
        } else {
            print(cmd_error);
        }
    } else {
        print(cmd_error);
    }

    print("\n> ");
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

