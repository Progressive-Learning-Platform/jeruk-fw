/*
    Copyright 2015 David Fritz, Brian Gordon, Wira Mulia

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
#include "procdefs.h"

// should be pretty close enough, 4 instructions / loop
void delay_ms(unsigned int milliseconds) {
    unsigned int t = milliseconds * (SYSTEM_CLOCK / 1000 / 4);
    __asm__("   move $t0, %0                 \n\r"
            "wdelay_loop:                    \n\r"
            "   beq $t0, $zero, wdelay_quit  \n\r"
            "   addiu $t0, $t0, -1           \n\r"
            "   j wdelay_loop                \n\r"
            "   nop                          \n\r"
            "wdelay_quit:                    \n\r"
            : : "r"(t) : "t0"
            );
}

void button_uart(char a, char b) {
    char stop = 0;
    while(!stop) {
        if(((char)read()) == 'q')
            stop = 1;
        if((PORTB & 1) == 0) {
            pchar(a);
            LEDS = 0b01;
            delay_ms(20);
        } else if((PORTB & 2) == 0) {
            pchar(b);
            LEDS = 0b10;
            delay_ms(20);
        }
    }
}

void party() {
    char stop = 0;
    while(!stop) {
        if(((char)read()) == 'q')
            stop = 1;
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