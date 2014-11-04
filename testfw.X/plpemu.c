#include <proc/p32mx270f256b.h>
#include "wio.h"

void emu_plp5(int entry) {
    int* pc = (int*) entry;
    char wordbuf[9];
    ascii_hex_word(wordbuf, entry);
    print("Entry point: ");
    print(wordbuf);

    /*
     * Fritz, your code goes here!
     *
     * Everything is stored as little-endian. If you read an int (32-bit) from
     * memory, the least significant byte will be fetched from the lowest
     * memory address. So you need to be mindful of the byte order to emulate
     * load and store word ops properly.
     *
     * Decoding instructions should be straight forward and no byte reordering
     * is needed.
     * 
     */

}

unsigned int fload_readword() {
    unsigned char buf;
    char i;
    unsigned int val = 0;
    for(i = 3; i >= 0; i--) {
        buf = blocking_read();
        val |= buf << (i*8);
    }
    return val;
}

// legacy loader, fload is big-endian / msb comes first
void fload() {
    char buf;
    unsigned int val;
    int* pc;
    int i;
    while(1) {
        buf = blocking_read();
        switch(buf) {
            case 'a':
                val = fload_readword();
                pc = (int*) val;
                pchar('f');
                break;
            case 'd':
                *pc = fload_readword();
                pc = (int*)((int)pc+4);
                pchar('f');
                break;
            case 'c':
                val = fload_readword();
                for(i = 0; i < val; i++) {
                    *pc = fload_readword();
                    pc = (int*)((int)pc+4);
                }
                pchar('f');
                break;
            case 'j':
                pchar('f');
                emu_plp5((int) pc);
                return;
            case 'p':
                val = fload_readword();
                // ignore preambles
                break;
            case 'v':
                print("PLPEMU-5.0");
                break;
        }
    }
}