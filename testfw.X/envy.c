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

// Include your processor's definitions
#include <proc/p32mx270f256b.h>

#include "wio.h"

#ifndef LEDS
#define LEDS PORTA
#endif

// PIC32MX1XX/2XX flash row and page sizes
#define ROW_SIZE_BYTES  128
#define PAGE_SIZE_BYTES 1024
#define ROW_SIZE_MASK   0x7F
#define PAGE_SIZE_MASK  0x3FF

// JERUK Flash loader routines
// Reference Document: DS61121F (PIC32 Family Reference Manual Chapter 5)
// - The unlock, erase, and programming sequence routines are taken from the
//   document

unsigned int NVMUnlock (unsigned int nvmop) {
    unsigned int status;

    // Suspend or Disable all Interrupts
    asm volatile ("di %0" : "=r" (status));

    // Enable Flash Write/Erase Operations and Select
    // Flash operation to perform
    NVMCON = nvmop;

    // Write Keys
    NVMKEY = 0xAA996655;
    NVMKEY = 0x556699AA;

    // Start the operation using the Set Register
    NVMCONSET = 0x8000;

    // Wait for operation to complete
    while (NVMCON & 0x8000);

    // Restore Interrupts
    if (status & 0x00000001) {
        asm volatile ("ei");
    } else {
        asm volatile ("di");
    }

    // Disable NVM write enable
    NVMCONCLR = 0x0004000;
    
    // Return WRERR and LVDERR Error Status Bits
    return (NVMCON & 0x3000);
}

unsigned int envy_write_word(void* address, unsigned int data) {
    unsigned int res;
    NVMDATA = data;
    NVMADDR = ((unsigned int) address & 0x1fffffff); // convert to physical
    res = NVMUnlock(0x4001);

    return res;
}

unsigned int envy_write_row(void* address, void* data) {
    unsigned int res;
    NVMADDR = ((unsigned int) address & 0x1fffffff); // convert to physical
    NVMSRCADDR = (unsigned int) data & 0x1fffffff;
    res = NVMUnlock(0x4003);

    return res;
}

unsigned int envy_erase_page(void* address) {
    unsigned int res;
    NVMADDR = ((unsigned int) address & 0x1fffffff); // convert to physical
    res = NVMUnlock(0x4004);
    
    return res;
}

// envy write routine. Each slot is 64KB with the first slot reserved for
// the bootloader. Size can not exceed 64KB.
void envy_write_stream(char slot, unsigned int size) {
    unsigned int byte_offset = 0;
    unsigned int base = 0xbd000000 + slot * 0x10000;
    unsigned int i = 0;
    unsigned int page_num = 0;
    unsigned int res;
    unsigned char checksum = 0;
    char* buf = (char*) 0xa0001000;
    void* page_ptr;
    void* row_ptr;

    // round up size to row size
    unsigned int fragment = (PAGE_SIZE_BYTES - (size & PAGE_SIZE_MASK)) & PAGE_SIZE_MASK;

    while(byte_offset < (size + fragment) && byte_offset < 0x10000) {
        if(byte_offset < size) {
            buf[byte_offset & PAGE_SIZE_MASK] = blocking_read();
        } else {
            buf[byte_offset & PAGE_SIZE_MASK] = 0xff; // empty location, write 1s until page boundary
        }
        byte_offset++;

        // host better wait for us now, or we'll miss the bytestream!
        if((byte_offset & PAGE_SIZE_MASK) == 0) {
            LEDS = 0x01;
            page_ptr = (void*) (base + PAGE_SIZE_BYTES*page_num);          
            res = envy_erase_page(page_ptr);
            if(res) {
                // erase error
                LEDS = 0b00011;
                pchar('e');
                while(1);
            }
            LEDS = 0x02;
            for(i = 0; i < (PAGE_SIZE_BYTES / ROW_SIZE_BYTES); i++) {
                row_ptr = (void*) ((unsigned int) page_ptr + i*ROW_SIZE_BYTES);
                res = envy_write_row(row_ptr,
                        (void*) ((unsigned int)buf + i*ROW_SIZE_BYTES));
                if(res) {
                    // row write error
                    LEDS = 0b00101;
                    pchar('e');
                    while(1);
                }
            }
            LEDS = 0x00;            
            page_num++;
            pchar('v');
        }
    }

    // we do checksum by actually reading off the flash
    byte_offset = base;
    while(byte_offset < base + size) {
        checksum += *((unsigned char*) byte_offset);
        byte_offset++;
    }
    pchar(checksum);
}

void envy() {
    char buf;
    char slot;
    unsigned int size;
    buf = blocking_read();
    switch(buf) {
        case 'n':
            slot = blocking_read();
            size = ((unsigned int) blocking_read() & 0xff) << 8;
            size |= ((unsigned int) blocking_read() & 0xff);
            envy_write_stream(slot, size);
            break;
        case 'j':
            break;
        case 'q':
            return;
    }
}

void envy_clear() {
    NVMUnlock(0x4000);
}