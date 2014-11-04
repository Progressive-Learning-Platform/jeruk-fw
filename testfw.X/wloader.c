// Wira's PIC32 Bootloader
#include "wio.h"

// global
volatile int* wload_pc;
unsigned char wload_checksum;

void wload_calc_checksum(int valbuf) {
    wload_checksum += (unsigned char)((valbuf & 0xff000000L)>>24);
    wload_checksum += (unsigned char)((valbuf & 0x00ff0000L)>>16);
    wload_checksum += (unsigned char)((valbuf & 0x0000ff00L)>>8);
    wload_checksum += (unsigned char)((valbuf & 0x000000ffL));
}

int readword_be() {
    char buf, i;
    int valbuf = 0;
    for(i = 0; i < 4; i++) {
        while((buf = blocking_read()) == ' ');
        valbuf |= (parse_ascii_hex(buf) << (i*8+4));
        while((buf = blocking_read()) == ' ');
        valbuf |= (parse_ascii_hex(buf) << (i*8));
    }
    wload_calc_checksum(valbuf);
    return valbuf;
}

int readword_le() {
    char buf, i;
    int valbuf = 0;
    for(i = 7; i >= 0; i--) {
        while((buf = blocking_read()) == ' ');
        valbuf |= (parse_ascii_hex(buf) << (i*4));
    }
    wload_calc_checksum(valbuf);
    return valbuf;
}

void wload() {
    char stop = 0;
    char buf;
    char wordbuf[9];
    char i;
    wload_checksum = 0; // reset checksum everytime wload routine is invoked
    int valbuf;
    wload_pc = (int*) 0;

    // tell remote host that we're in wload
    pchar('k');
    
    // begin bootloader loop
    while(!stop) {
        buf = blocking_read();
        switch(buf) {
            case 'a': // set loader pc
                valbuf = readword_le();
                wload_pc = (int*) valbuf;
                break;
            case 'w': // write a word little endian
                valbuf = readword_le();
                *wload_pc = valbuf;
                wload_pc = (int*)((int) wload_pc + 4);
                break;
            case 'W': // write a word big endian
                valbuf = readword_be();
                *wload_pc = valbuf;
                wload_pc = (int*)((int) wload_pc + 4);
                break;
            case 'c': // get checksum
                pchar(ascii_byte_h(wload_checksum));
                pchar(ascii_byte_l(wload_checksum));
                break;
            case 'j': // jump to current loader pc
                valbuf = (int) wload_pc;
                __asm__("move $s0, %0\n"
                        "jr $s0\n"
                        "nop"
                        : : "r"(valbuf) : "s0");
                break;
            case 'p': // print pc
                ascii_hex_word(wordbuf, (int) wload_pc);
                print(wordbuf);
                break;
            case 'q': //quit bootloader and hand off control to caller
                stop = 1;
                print("\nwload exit");
                break;
            case 0x20:
            case 0x0a:
            case 0x0d:
                break;
            default:
                print("\nwload error: unknown directive: ");
                pchar(buf);
                stop = 1;
        }
    }
}
